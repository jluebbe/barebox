/*
 * pinctrl.c - barebox pinctrl support
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>
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
#include <common.h>
#include <malloc.h>
#include <pinctrl.h>

static LIST_HEAD(pinctrl_list);

static struct pinctrl_device *find_pinctrl(struct device_node *node)
{
	struct pinctrl_device *pdev;

	list_for_each_entry(pdev, &pinctrl_list, list)
		if (pdev->node == node)
			return pdev;
	return NULL;
}

static int pinctrl_config_one(struct device_node *np)
{
	struct pinctrl_device *pdev;
	struct device_node *pinctrl_node = np;

	while (1) {
		pinctrl_node = pinctrl_node->parent;
		if (!pinctrl_node)
			return -ENODEV;
		pdev = find_pinctrl(pinctrl_node);
		if (pdev)
			break;
	}

	return pdev->ops->set_state(pdev, np);
}

int pinctrl_select_state(struct device_d *dev, const char *name)
{
	int state, ret;
	char *propname;
	struct property *prop;
	const __be32 *list;
	int size, config;
	phandle phandle;
	struct device_node *np_config, *np;
	const char *statename;

	np = dev->device_node;
	if (!np)
		return 0;

	if (!of_find_property(np, "pinctrl-0", NULL))
		return 0;

	/* For each defined state ID */
	for (state = 0; ; state++) {
		/* Retrieve the pinctrl-* property */
		propname = asprintf("pinctrl-%d", state);
		prop = of_find_property(np, propname, NULL);
		free(propname);

		if (!prop) {
			ret = -ENODEV;
			break;
		}

		size = prop->length;

		list = prop->value;
		size /= sizeof(*list);

		/* Determine whether pinctrl-names property names the state */
		ret = of_property_read_string_index(np, "pinctrl-names",
						    state, &statename);
		/*
		 * If not, statename is just the integer state ID. But rather
		 * than dynamically allocate it and have to free it later,
		 * just point part way into the property name for the string.
		 */
		if (ret < 0) {
			/* strlen("pinctrl-") == 8 */
			statename = prop->name + 8;
		}

		if (strcmp(name, statename))
			continue;

		/* For every referenced pin configuration node in it */
		for (config = 0; config < size; config++) {
			phandle = be32_to_cpup(list++);

			/* Look up the pin configuration node */
			np_config = of_find_node_by_phandle(phandle);
			if (!np_config) {
				pr_err("prop %s %s index %i invalid phandle\n",
					np->full_name, prop->name, config);
				ret = -EINVAL;
				goto err;
			}

			/* Parse the node */
			ret = pinctrl_config_one(np_config);
			if (ret < 0)
				goto err;
		}

		return 0;
	}
err:
	return ret;
}

int pinctrl_select_state_default(struct device_d *dev)
{
	return pinctrl_select_state(dev, "default");
}

int pinctrl_register(struct pinctrl_device *pdev)
{
	BUG_ON(!pdev->dev->device_node);

	list_add_tail(&pdev->list, &pinctrl_list);

	pdev->node = pdev->dev->device_node;

	pinctrl_select_state_default(pdev->dev);

	return 0;
}

void pinctrl_unregister(struct pinctrl_device *pdev)
{
	list_del(&pdev->list);
}
