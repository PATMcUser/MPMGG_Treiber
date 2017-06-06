#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xe5d27530, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x9aa74525, __VMLINUX_SYMBOL_STR(gameport_unregister_driver) },
	{ 0x2162a06f, __VMLINUX_SYMBOL_STR(__gameport_register_driver) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x2a5c19c8, __VMLINUX_SYMBOL_STR(input_free_device) },
	{ 0x28bf52cb, __VMLINUX_SYMBOL_STR(input_register_device) },
	{ 0x676bbc0f, __VMLINUX_SYMBOL_STR(_set_bit) },
	{ 0x2844a557, __VMLINUX_SYMBOL_STR(input_set_abs_params) },
	{ 0xb81960ca, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x6ebebdc4, __VMLINUX_SYMBOL_STR(input_allocate_device) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xcf7046e5, __VMLINUX_SYMBOL_STR(gameport_open) },
	{ 0x3de68ba, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xa80ce3db, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x8f678b07, __VMLINUX_SYMBOL_STR(__stack_chk_guard) },
	{ 0x595db0fb, __VMLINUX_SYMBOL_STR(input_event) },
	{ 0xdf8e5120, __VMLINUX_SYMBOL_STR(gameport_start_polling) },
	{ 0xbd82f59f, __VMLINUX_SYMBOL_STR(gameport_stop_polling) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x5396aaa5, __VMLINUX_SYMBOL_STR(input_unregister_device) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xe6abe033, __VMLINUX_SYMBOL_STR(gameport_close) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=gameport";


MODULE_INFO(srcversion, "406A334D6EED6889D011DF7");
