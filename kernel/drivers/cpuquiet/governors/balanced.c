/*
 * Copyright (c) 2012 NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/cpuquiet.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/pm_qos_params.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <asm/cputime.h>

#define CPUNAMELEN 8

typedef enum {
	CPU_SPEED_BALANCED,
	CPU_SPEED_BIASED,
	CPU_SPEED_SKEWED,
} CPU_SPEED_BALANCE;

typedef enum {
	IDLE,
	DOWN,
	UP,
} BALANCED_STATE;

struct idle_info {
	u64 idle_last;
	u64 last_timestamp;
	u64 idle_current;
	u64 timestamp;
};

static DEFINE_PER_CPU(struct idle_info, idleinfo);
static DEFINE_PER_CPU(unsigned int, cpu_load);

static struct timer_list load_timer;
static bool load_timer_active;
struct balanced_attribute {
	struct attribute attr;
	ssize_t (*show)(struct balanced_attribute *attr, char *buf);
	ssize_t (*store)(struct balanced_attribute *attr, const char *buf,
				size_t count);
	unsigned long *param;
};

#define BALANCED_ATTRIBUTE(_name, _mode) \
	static struct balanced_attribute _name ## _attr = {		\
		.attr = {.name = __stringify(_name), .mode = _mode },	\
		.show	= show_attribute,				\
		.store	= store_attribute,				\
		.param	= &_name,					\
}

/* configurable parameters */
static unsigned long balance_level = 75;
static unsigned long idle_bottom_freq;
static unsigned long idle_top_freq;
static unsigned long up_delay;
static unsigned long down_delay;

static struct workqueue_struct *balanced_wq;
static struct delayed_work balanced_work;
static BALANCED_STATE balanced_state;
static struct kobject *balanced_kobject;

static void calculate_load_timer(unsigned long data)
{
	int i;
	u64 idle_time, elapsed_time;

	if (!load_timer_active)
		return;

	for_each_online_cpu(i) {
		struct idle_info *iinfo = &per_cpu(idleinfo, i);
		unsigned int *load = &per_cpu(cpu_load, i);

		iinfo->idle_last = iinfo->idle_current;
		iinfo->last_timestamp = iinfo->timestamp;
		iinfo->idle_current =
			get_cpu_idle_time_us(i, &iinfo->timestamp);
		elapsed_time = iinfo->timestamp - iinfo->last_timestamp;

		idle_time = iinfo->idle_current - iinfo->idle_last;
		idle_time *= 100;
		do_div(idle_time, elapsed_time);
		*load = 100 - idle_time;
	}
	mod_timer(&load_timer, jiffies + msecs_to_jiffies(100));
}

static void start_load_timer(void)
{
	int i;

	if (load_timer_active)
		return;

	load_timer_active = true;

	for_each_online_cpu(i) {
		struct idle_info *iinfo = &per_cpu(idleinfo, i);

		iinfo->idle_current =
			get_cpu_idle_time_us(i, &iinfo->timestamp);
	}
	mod_timer(&load_timer, jiffies + msecs_to_jiffies(100));
}

static void stop_load_timer(void)
{
	if (!load_timer_active)
		return;

	load_timer_active = false;
	del_timer(&load_timer);
}

static unsigned int get_slowest_cpu_n(void)
{
	unsigned int cpu = nr_cpu_ids;
	unsigned long minload = ULONG_MAX;
	int i;

	for_each_online_cpu(i) {
		unsigned int *load = &per_cpu(cpu_load, i);

		if ((i > 0) && (minload > *load)) {
			cpu = i;
			minload = *load;
		}
	}

	return cpu;
}

static unsigned int cpu_highest_speed(void)
{
	unsigned int maxload = 0;
	int i;

	for_each_online_cpu(i) {
		unsigned int *load = &per_cpu(cpu_load, i);

		maxload = max(maxload, *load);
	}

	return maxload;
}

static unsigned int count_slow_cpus(unsigned int limit)
{
	unsigned int cnt = 0;
	int i;

	for_each_online_cpu(i) {
		unsigned int *load = &per_cpu(cpu_load, i);

		if (*load <= limit)
			cnt++;
	}

	return cnt;
}

static CPU_SPEED_BALANCE balanced_speed_balance(void)
{
	unsigned long highest_speed = cpu_highest_speed();
	unsigned long balanced_speed = highest_speed * balance_level / 100;
	unsigned long skewed_speed = balanced_speed / 2;
	unsigned int nr_cpus = num_online_cpus();
	unsigned int max_cpus = pm_qos_request(PM_QOS_MAX_ONLINE_CPUS) ? : 4;

	/* balanced: freq targets for all CPUs are above 50% of highest speed
	   biased: freq target for at least one CPU is below 50% threshold
	   skewed: freq targets for at least 2 CPUs are below 25% threshold */
	if (count_slow_cpus(skewed_speed) >= 2 || nr_cpus > max_cpus)
		return CPU_SPEED_SKEWED;

	if (count_slow_cpus(balanced_speed) >= 1 || nr_cpus == max_cpus)
		return CPU_SPEED_BIASED;

	return CPU_SPEED_BALANCED;
}

static void balanced_work_func(struct work_struct *work)
{
	bool up = false;
	unsigned int cpu = nr_cpu_ids;
	CPU_SPEED_BALANCE balance;

	switch (balanced_state) {
	case IDLE:
		break;
	case DOWN:
		cpu = get_slowest_cpu_n();
		if (cpu < nr_cpu_ids) {
			up = false;
			queue_delayed_work(balanced_wq,
						 &balanced_work, down_delay);
		} else
			stop_load_timer();
		break;
	case UP:
		balance = balanced_speed_balance();
		switch (balance) {

		/* cpu speed is up and balanced - one more on-line */
		case CPU_SPEED_BALANCED:
			cpu = cpumask_next_zero(0, cpu_online_mask);
			if (cpu < nr_cpu_ids)
				up = true;
			break;
		/* cpu speed is up, but skewed - remove one core */
		case CPU_SPEED_SKEWED:
			cpu = get_slowest_cpu_n();
			if (cpu < nr_cpu_ids)
				up = false;
			break;
		/* cpu speed is up, but under-utilized - do nothing */
		case CPU_SPEED_BIASED:
		default:
			break;
		}
		queue_delayed_work(
			balanced_wq, &balanced_work, up_delay);
		break;
	default:
		pr_err("%s: invalid cpuquiet balanced governor state %d\n",
		       __func__, balanced_state);
	}

	if (cpu < nr_cpu_ids) {
		if (up)
			cpuquiet_wake_cpu(cpu);
		else
			cpuquiet_quiesence_cpu(cpu);
	}
}

static int balanced_cpufreq_transition(struct notifier_block *nb,
	unsigned long state, void *data)
{
	struct cpufreq_freqs *freqs = data;
	unsigned long cpu_freq;

	if (state == CPUFREQ_POSTCHANGE || state == CPUFREQ_RESUMECHANGE) {
		cpu_freq = freqs->new;

		switch (balanced_state) {
		case IDLE:
			if (cpu_freq > idle_top_freq) {
				balanced_state = UP;
				queue_delayed_work(
					balanced_wq, &balanced_work, up_delay);
				start_load_timer();
			} else if (cpu_freq <= idle_bottom_freq) {
				balanced_state = DOWN;
				queue_delayed_work(
					balanced_wq, &balanced_work,
					down_delay);
				start_load_timer();
			}
			break;
		case DOWN:
			if (cpu_freq > idle_top_freq) {
				balanced_state = UP;
				queue_delayed_work(
					balanced_wq, &balanced_work, up_delay);
				start_load_timer();
			}
			break;
		case UP:
			if (cpu_freq <= idle_bottom_freq) {
				balanced_state = DOWN;
				queue_delayed_work(balanced_wq,
					&balanced_work, down_delay);
				start_load_timer();
			}
			break;
		default:
			pr_err("%s: invalid tegra hotplug state %d\n",
				__func__, balanced_state);
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block balanced_cpufreq_nb = {
	.notifier_call = balanced_cpufreq_transition,
};

static ssize_t show_attribute(struct balanced_attribute *battr, char *buf)
{
	return sprintf(buf, "%lu\n", *(battr->param));
}

static ssize_t store_attribute(struct balanced_attribute *battr,
					const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 0, &val);
	if (err < 0)
		return err;

	*(battr->param) = val;

	return count;
}

static ssize_t balanced_sysfs_store(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	struct balanced_attribute *battr =
		 container_of(attr, struct balanced_attribute, attr);

	if (battr->store)
		return battr->store(battr, buf, count);

	return -EINVAL;
}

static ssize_t balanced_sysfs_show(struct kobject *kobj,
			struct attribute *attr, char *buf)
{
	struct balanced_attribute *battr =
		 container_of(attr, struct balanced_attribute, attr);

	return battr->show(battr, buf);
}

BALANCED_ATTRIBUTE(balance_level, 0644);
BALANCED_ATTRIBUTE(idle_bottom_freq, 0644);
BALANCED_ATTRIBUTE(idle_top_freq, 0644);
BALANCED_ATTRIBUTE(up_delay, 0644);
BALANCED_ATTRIBUTE(down_delay, 0644);

static struct attribute *balanced_attributes[] = {
	&balance_level_attr.attr,
	&idle_bottom_freq_attr.attr,
	&idle_top_freq_attr.attr,
	&up_delay_attr.attr,
	&down_delay_attr.attr,
	NULL,
};

static const struct sysfs_ops balanced_sysfs_ops = {
	.show = balanced_sysfs_show,
	.store = balanced_sysfs_store,
};

static struct kobj_type ktype_balanced = {
	.sysfs_ops = &balanced_sysfs_ops,
	.default_attrs = balanced_attributes,
};

static int balanced_sysfs(void)
{
	int err;

	balanced_kobject = kzalloc(sizeof(*balanced_kobject),
				GFP_KERNEL);

	if (!balanced_kobject)
		return -ENOMEM;

	err = cpuquiet_kobject_init(balanced_kobject, &ktype_balanced,
				"balanced");

	if (err)
		kfree(balanced_kobject);

	return err;
}

static void balanced_stop(void)
{

	/*
	   first unregister the notifiers. This ensures the governor state
	   can't be modified by a cpufreq transition
	*/
	cpufreq_unregister_notifier(&balanced_cpufreq_nb,
		CPUFREQ_TRANSITION_NOTIFIER);

	/* now we can force the governor to be idle */
	balanced_state = IDLE;
	cancel_delayed_work_sync(&balanced_work);
	destroy_workqueue(balanced_wq);
	del_timer(&load_timer);

	kobject_put(balanced_kobject);
}

static int balanced_start(void)
{
	int err, count;
	struct cpufreq_frequency_table *table;

	err = balanced_sysfs();
	if (err)
		return err;

	balanced_wq = alloc_workqueue("cpuquiet-balanced",
			WQ_UNBOUND | WQ_RESCUER | WQ_FREEZABLE, 1);
	if (!balanced_wq)
		return -ENOMEM;

	INIT_DELAYED_WORK(&balanced_work, balanced_work_func);

	up_delay = msecs_to_jiffies(1000);
	down_delay = msecs_to_jiffies(2000);

	table = cpufreq_frequency_get_table(0);
	for (count = 0; table[count].frequency != CPUFREQ_TABLE_END; count++)
		;

	idle_top_freq = table[(count / 2) - 1].frequency;
	idle_bottom_freq = table[(count / 2) - 2].frequency;

	cpufreq_register_notifier(&balanced_cpufreq_nb,
		CPUFREQ_TRANSITION_NOTIFIER);

	init_timer(&load_timer);
	load_timer.function = calculate_load_timer;

	return 0;
}

struct cpuquiet_governor balanced_governor = {
	.name		= "balanced",
	.start		= balanced_start,
	.stop		= balanced_stop,
	.owner		= THIS_MODULE,
};

static int __init init_balanced(void)
{
	return cpuquiet_register_governor(&balanced_governor);
}

static void __exit exit_balanced(void)
{
	cpuquiet_unregister_governor(&balanced_governor);
}

MODULE_LICENSE("GPL");
module_init(init_balanced);
module_exit(exit_balanced);

