# STM32_SDIO_Simplified

官方的sdio驱动实在是又臭又长, 居然有3000行, 实在是不能忍受. 自己精简了一下, 只有不到1000行了.

实现了单/多扇区读/写, 包括了DMA配置, 未包括GPIO配置.

支持STM32F103和STM32F401, 只有DMA配置不同, 其他基本相同.