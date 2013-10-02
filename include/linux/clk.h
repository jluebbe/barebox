/*
 *  linux/include/linux/clk.h
 *
 *  Copyright (C) 2004 ARM Limited.
 *  Written by Deep Blue Solutions Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __LINUX_CLK_H
#define __LINUX_CLK_H

#include <linux/err.h>

struct device_d;

/*
 * The base API.
 */

/*
 * struct clk - an machine class defined object / cookie.
 */
struct clk;

#ifdef CONFIG_HAVE_CLK

/**
 * clk_get - lookup and obtain a reference to a clock producer.
 * @dev: device for clock "consumer"
 * @id: clock comsumer ID
 *
 * Returns a struct clk corresponding to the clock producer, or
 * valid IS_ERR() condition containing errno.  The implementation
 * uses @dev and @id to determine the clock consumer, and thereby
 * the clock producer.  (IOW, @id may be identical strings, but
 * clk_get may return different clock producers depending on @dev.)
 *
 * Drivers must assume that the clock source is not enabled.
 *
 * clk_get should not be called from within interrupt context.
 */
struct clk *clk_get(struct device_d *dev, const char *id);

/**
 * clk_enable - inform the system when the clock source should be running.
 * @clk: clock source
 *
 * If the clock can not be enabled/disabled, this should return success.
 *
 * Returns success (0) or negative errno.
 */
int clk_enable(struct clk *clk);

/**
 * clk_disable - inform the system when the clock source is no longer required.
 * @clk: clock source
 *
 * Inform the system that a clock source is no longer required by
 * a driver and may be shut down.
 *
 * Implementation detail: if the clock source is shared between
 * multiple drivers, clk_enable() calls must be balanced by the
 * same number of clk_disable() calls for the clock source to be
 * disabled.
 */
void clk_disable(struct clk *clk);

/**
 * clk_get_rate - obtain the current clock rate (in Hz) for a clock source.
 *		  This is only valid once the clock source has been enabled.
 * @clk: clock source
 */
unsigned long clk_get_rate(struct clk *clk);

/**
 * clk_put	- "free" the clock source
 * @clk: clock source
 *
 * Note: drivers must ensure that all clk_enable calls made on this
 * clock source are balanced by clk_disable calls prior to calling
 * this function.
 *
 * clk_put should not be called from within interrupt context.
 */
void clk_put(struct clk *clk);


/*
 * The remaining APIs are optional for machine class support.
 */


/**
 * clk_round_rate - adjust a rate to the exact rate a clock can provide
 * @clk: clock source
 * @rate: desired clock rate in Hz
 *
 * Returns rounded clock rate in Hz, or negative errno.
 */
long clk_round_rate(struct clk *clk, unsigned long rate);

/**
 * clk_set_rate - set the clock rate for a clock source
 * @clk: clock source
 * @rate: desired clock rate in Hz
 *
 * Returns success (0) or negative errno.
 */
int clk_set_rate(struct clk *clk, unsigned long rate);

/**
 * clk_set_parent - set the parent clock source for this clock
 * @clk: clock source
 * @parent: parent clock source
 *
 * Returns success (0) or negative errno.
 */
int clk_set_parent(struct clk *clk, struct clk *parent);

/**
 * clk_get_parent - get the parent clock source for this clock
 * @clk: clock source
 *
 * Returns struct clk corresponding to parent clock source, or
 * valid IS_ERR() condition containing errno.
 */
struct clk *clk_get_parent(struct clk *clk);

/**
 * clk_get_sys - get a clock based upon the device name
 * @dev_id: device name
 * @con_id: connection ID
 *
 * Returns a struct clk corresponding to the clock producer, or
 * valid IS_ERR() condition containing errno.  The implementation
 * uses @dev_id and @con_id to determine the clock consumer, and
 * thereby the clock producer. In contrast to clk_get() this function
 * takes the device name instead of the device itself for identification.
 *
 * Drivers must assume that the clock source is not enabled.
 *
 * clk_get_sys should not be called from within interrupt context.
 */
struct clk *clk_get_sys(const char *dev_id, const char *con_id);

/**
 * clk_add_alias - add a new clock alias
 * @alias: name for clock alias
 * @alias_dev_name: device name
 * @id: platform specific clock name
 * @dev: device
 *
 * Allows using generic clock names for drivers by adding a new alias.
 * Assumes clkdev, see clkdev.h for more info.
 */
int clk_add_alias(const char *alias, const char *alias_dev_name, char *id,
			struct device_d *dev);

#else

static inline struct clk *clk_get(struct device_d *dev, const char *id)
{
	return NULL;
}

static inline int clk_enable(struct clk *clk)
{
	return 0;
}

static inline void clk_disable(struct clk *clk)
{
}

static inline unsigned long clk_get_rate(struct clk *clk)
{
	return 0;
}

static inline void clk_put(struct clk *clk)
{
}

static inline long clk_round_rate(struct clk *clk, unsigned long rate)
{
	return 0;
}

static inline int clk_set_rate(struct clk *clk, unsigned long rate)
{
	return 0;
}
#endif

#ifdef CONFIG_COMMON_CLK
struct clk_ops {
	int		(*enable)(struct clk *clk);
	void		(*disable)(struct clk *clk);
	int		(*is_enabled)(struct clk *clk);
	unsigned long	(*recalc_rate)(struct clk *clk,
					unsigned long parent_rate);
	long		(*round_rate)(struct clk *clk, unsigned long,
					unsigned long *);
	int		(*set_parent)(struct clk *clk, u8 index);
	int		(*get_parent)(struct clk *clk);
	int		(*set_rate)(struct clk *clk, unsigned long,
				    unsigned long);
};

struct clk {
	const struct clk_ops *ops;
	int enable_count;
	struct list_head list;
	const char *name;
	const char **parent_names;
	int num_parents;

	struct clk **parents;
	unsigned long flags;
};

struct clk_div_table {
	unsigned int	val;
	unsigned int	div;
};

struct clk *clk_fixed(const char *name, int rate);

struct clk_divider {
	struct clk clk;
	u8 shift;
	u8 width;
	void __iomem *reg;
	const char *parent;
#define CLK_DIVIDER_ONE_BASED	(1 << 0)
	unsigned flags;
};

extern struct clk_ops clk_divider_ops;

struct clk *clk_divider(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width);
struct clk *clk_divider_one_based(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width);
struct clk *clk_divider_table(const char *name,
		const char *parent, void __iomem *reg, u8 shift, u8 width,
		const struct clk_div_table *table);
struct clk *clk_fixed_factor(const char *name,
		const char *parent, unsigned int mult, unsigned int div);

struct clk *clk_mux_alloc(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parents, u8 num_parents);
void clk_mux_free(struct clk *clk_mux);
struct clk *clk_mux(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parents, u8 num_parents);

struct clk *clk_gate_alloc(const char *name, const char *parent,
		void __iomem *reg, u8 shift);
void clk_gate_free(struct clk *clk_gate);
struct clk *clk_gate(const char *name, const char *parent, void __iomem *reg,
		u8 shift);
struct clk *clk_gate_inverted(const char *name, const char *parent, void __iomem *reg,
		u8 shift);
int clk_is_enabled(struct clk *clk);

int clk_is_enabled_always(struct clk *clk);

int clk_register(struct clk *clk);

struct clk *clk_lookup(const char *name);

void clk_dump(int verbose);

#endif

struct device_node;
struct of_phandle_args;

#if defined(CONFIG_OFTREE) && defined(CONFIG_COMMON_CLK_OF_PROVIDER)
int of_clk_add_provider(struct device_node *np,
			struct clk *(*clk_src_get)(struct of_phandle_args *args,
						   void *data),
			void *data);
void of_clk_del_provider(struct device_node *np);

struct clk_onecell_data {
	struct clk **clks;
	unsigned int clk_num;
};
struct clk *of_clk_src_onecell_get(struct of_phandle_args *clkspec, void *data);
struct clk *of_clk_src_simple_get(struct of_phandle_args *clkspec, void *data);

struct clk *of_clk_get(struct device_node *np, int index);
struct clk *of_clk_get_by_name(struct device_node *np, const char *name);
struct clk *of_clk_get_from_provider(struct of_phandle_args *clkspec);
#else
static inline struct clk *of_clk_get(struct device_node *np, int index)
{
	return ERR_PTR(-ENOENT);
}
static inline struct clk *of_clk_get_by_name(struct device_node *np,
					     const char *name)
{
	return ERR_PTR(-ENOENT);
}
#endif

#endif
