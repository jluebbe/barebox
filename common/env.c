/*
 * env.c - environment variables storage
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file
 * @brief Environment support
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <environment.h>

static struct env_context root = {
	.local = LIST_HEAD_INIT(root.local),
	.global = LIST_HEAD_INIT(root.global),
};

static struct env_context *context = &root;

/**
 * Remove a list of environment variables
 * @param[in] v Variable anchor to remove
 */
static void free_context(struct env_context *c)
{
	struct variable_d *v, *tmp;

	list_for_each_entry_safe(v, tmp, &c->local, list) {
		free(v->name);
		free(v->data);
		list_del(&v->list);
		free(v);
	}

	list_for_each_entry_safe(v, tmp, &c->global, list) {
		free(v->name);
		free(v->data);
		list_del(&v->list);
		free(v);
	}

	free(c);
}

/** Read back current context */
struct env_context *get_current_context(void)
{
	return context;
}
EXPORT_SYMBOL(get_current_context);


/**
 * Create a new variable context and put it on the stack
 */
int env_push_context(void)
{
	struct env_context *c = xzalloc(sizeof(struct env_context));

	INIT_LIST_HEAD(&c->local);
	INIT_LIST_HEAD(&c->global);

	c->parent = context;
	context = c;

	return 0;
}

/**
 * free current variable context and restore the previous one
 */
int env_pop_context(void)
{
	struct env_context *c = context;

	if (context->parent) {
		c = context->parent;
		free_context(context);
		context = c;
		return 0;
	}
	return -EINVAL;
}

/**
 * Return variable's value
 * @param[in] var Variable of interest
 * @return Value as text
 */
char *var_val(struct variable_d *var)
{
	return var->data;
}

/**
 * Return variable's name
 * @param[in] var Variable of interest
 * @return Name as text
 */
char *var_name(struct variable_d *var)
{
	return var->name;
}

static const char *getenv_raw(struct list_head *l, const char *name)
{
	struct variable_d *v;

	list_for_each_entry(v, l, list) {
		if (!strcmp(var_name(v), name))
			return var_val(v);
	}

	return NULL;
}

const char *getenv (const char *name)
{
	struct env_context *c;
	const char *val;

	if (strchr(name, '.')) {
		const char *ret = NULL;
		char *devstr = strdup(name);
		char *par = strchr(devstr, '.');
		struct device_d *dev;
		*par = 0;
		dev = get_device_by_name(devstr);
		if (dev) {
				par++;
				ret = dev_get_param(dev, par);
		}
		free(devstr);
		return ret;
	}

	c = context;

	val = getenv_raw(&c->local, name);
	if (val)
		return val;

	while (c) {
		val = getenv_raw(&c->global, name);
		if (val)
			return val;
		c = c->parent;
	}
	return NULL;
}
EXPORT_SYMBOL(getenv);

static int setenv_raw(struct list_head *l, const char *name, const char *value)
{
	struct variable_d *v;

	list_for_each_entry(v, l, list) {
		if (!strcmp(v->name, name)) {
			if (value) {
				free(v->data);
				v->data = xstrdup(value);

				return 0;
			} else {
				list_del(&v->list);
				free(v->name);
				free(v->data);
				free(v);

				return 0;
			}
		}
	}

	if (value) {
		v = xzalloc(sizeof(*v));
		v->name = xstrdup(name);
		v->data = xstrdup(value);
		list_add_tail(&v->list, l);
	}

	return 0;
}

int setenv(const char *_name, const char *value)
{
	char *name = strdup(_name);
	char *par;
	int ret = 0;
	struct list_head *list;

	if (value && !*value)
		value = NULL;


	if ((par = strchr(name, '.'))) {
		struct device_d *dev;

		*par++ = 0;
		dev = get_device_by_name(name);
		if (dev)
			ret = dev_set_param(dev, par, value);
		else
			ret = -ENODEV;

		errno = -ret;

		if (ret < 0)
			perror("set parameter");

		goto out;
	}

	if (getenv_raw(&context->global, name))
		list = &context->global;
	else
		list = &context->local;

	ret = setenv_raw(list, name, value);
out:
	free(name);

	return ret;
}
EXPORT_SYMBOL(setenv);

int export(const char *varname)
{
	const char *val = getenv_raw(&context->local, varname);

	if (val) {
		setenv_raw(&context->global, varname, val);
		setenv_raw(&context->local, varname, NULL);
	}
	return 0;
}
EXPORT_SYMBOL(export);

void export_env_ull(const char *name, unsigned long long val)
{
	char *valstr = asprintf("%lld", val);

	setenv(name, valstr);
	export(name);

	kfree(valstr);
}
EXPORT_SYMBOL(export_env_ull);

/*
 * Like regular getenv, but never returns an empty string.
 * If the string is empty, NULL is returned instead
 */
const char *getenv_nonempty(const char *var)
{
	const char *val = getenv(var);

	if (val && *val)
		return val;

	return NULL;
}
EXPORT_SYMBOL(getenv_nonempty);

int getenv_ull(const char *var , unsigned long long *val)
{
	const char *valstr = getenv(var);

	if (!valstr || !*valstr)
		return -EINVAL;

	*val = simple_strtoull(valstr, NULL, 0);

	return 0;
}
EXPORT_SYMBOL(getenv_ull);

int getenv_ul(const char *var , unsigned long *val)
{
	const char *valstr = getenv(var);

	if (!valstr || !*valstr)
		return -EINVAL;

	*val = simple_strtoul(valstr, NULL, 0);

	return 0;
}
EXPORT_SYMBOL(getenv_ul);

int getenv_uint(const char *var , unsigned int *val)
{
	const char *valstr = getenv(var);

	if (!valstr || !*valstr)
		return -EINVAL;

	*val = simple_strtoul(valstr, NULL, 0);

	return 0;
}
EXPORT_SYMBOL(getenv_uint);

int getenv_bool(const char *var, int *val)
{
	const char *valstr = getenv(var);

	if (!valstr || !*valstr)
		return -EINVAL;

	if (!*valstr)
		*val = false;
	else if (*valstr == '0')
		*val = false;
	else
		*val = true;

	return 0;
}
EXPORT_SYMBOL(getenv_bool);
