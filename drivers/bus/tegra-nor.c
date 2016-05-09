/*
 * Driver for Nvidia NOR Flash bus a.k.a SNOR/GMI.
 *
 * Copyright (C) 206 Host Mobility AB
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>


#define TEGRA_NOR_CONFIG			0x00
#define TEGRA_NOR_STATUS			0x04
#define TEGRA_NOR_ADDR_PTR			0x08
#define TEGRA_NOR_AHB_ADDR_PTR		0x0c
#define TEGRA_NOR_TIMING0			0x10
#define TEGRA_NOR_TIMING1			0x14
#define TEGRA_NOR_MIO_CONFIG		0x18
#define TEGRA_NOR_MIO_TIMING		0x1c
#define TEGRA_NOR_DMA_CONFIG		0x20
#define TEGRA_NOR_CS_MUX_CONFIG		0x24


#define TEGRA_NOR_CONFIG_GO				BIT(31)
#define TEGRA_NOR_CONFIG_MUX			BIT(28)
#define TEGRA_NOR_CONFIG_ADV_POL		BIT(22)
#define TEGRA_NOR_CONFIG_SNOR_CS(n)		((n) << 4)

static const struct of_device_id nor_id_table[] = {
	/* Tegra30 */
	{ .compatible = "nvidia,tegra30-nor", .data = NULL, },
	/* Tegra20 */
	{ .compatible = "nvidia,tegra20-nor", .data = NULL, },

	{ }
};
MODULE_DEVICE_TABLE(of, nor_id_table);


static int __init nor_parse_dt(struct platform_device *pdev,
				void __iomem *base)
{
	int ret = 0;
	u32 nor_config;

	writel(TEGRA_NOR_CONFIG_SNOR_CS(4) | TEGRA_NOR_CONFIG_MUX |
		TEGRA_NOR_CONFIG_ADV_POL, base + TEGRA_NOR_CONFIG);

	nor_config = readl(base + TEGRA_NOR_CONFIG);
	nor_config |= TEGRA_NOR_CONFIG_GO;
	writel(nor_config, base + TEGRA_NOR_CONFIG);

	if (of_get_child_count(pdev->dev.of_node))
		ret = of_platform_populate(pdev->dev.of_node,
				   of_default_bus_match_table,
				   NULL, &pdev->dev);

	if (ret)
		dev_err(&pdev->dev, "%s fail to create devices.\n",
			pdev->dev.of_node->full_name);
	return ret;
}

static int __init nor_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct clk *clk;
	void __iomem *base;
	int ret;

	/* get the resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	/* get the clock */
	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_prepare_enable(clk);
	if (ret)
		return ret;

	/* parse the device node */
	ret = nor_parse_dt(pdev, base);
	if (ret)
		clk_disable_unprepare(clk);
	else
		dev_info(&pdev->dev, "Driver registered.\n");

	return ret;
}

static struct platform_driver nor_driver = {
	.driver = {
		.name		= "tegra-nor",
		.of_match_table	= nor_id_table,
	},
};
module_platform_driver_probe(nor_driver, nor_probe);

MODULE_AUTHOR("Host Mobiltiy AB");
MODULE_DESCRIPTION("Nvidia Tegra20/30 NOR/SNOR/GMI Controller Driver");
MODULE_LICENSE("GPL");
