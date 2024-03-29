/*
 *
 * (C) COPYRIGHT 2012-2016 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc.
 *
 */



#include <mali_kbase.h>
#ifdef ENABLE_MTK_MEMINFO
#include "platform/mtk_platform_common.h"
#endif /* ENABLE_MTK_MEMINFO */

#ifdef CONFIG_DEBUG_FS
/** Show callback for the @c gpu_memory debugfs file.
 *
 * This function is called to get the contents of the @c gpu_memory debugfs
 * file. This is a report of current gpu memory usage.
 *
 * @param sfile The debugfs entry
 * @param data Data associated with the entry
 *
 * @return 0 if successfully prints data in debugfs entry file
 *         -1 if it encountered an error
 */

static int kbasep_gpu_memory_seq_show(struct seq_file *sfile, void *data)
{
	struct list_head *entry;
	const struct list_head *kbdev_list;

#ifdef ENABLE_MTK_MEMINFO
	ssize_t mtk_kbase_gpu_meminfo_index;

	mtk_kbase_reset_gpu_meminfo();
#endif /* ENABLE_MTK_MEMINFO */

	kbdev_list = kbase_dev_list_get();
	list_for_each(entry, kbdev_list) {
		struct kbase_device *kbdev = NULL;
		struct kbasep_kctx_list_element *element;

		kbdev = list_entry(entry, struct kbase_device, entry);
		/* output the total memory usage and cap for this device */
		seq_printf(sfile, "%-16s  %10u\n",
				kbdev->devname,
				atomic_read(&(kbdev->memdev.used_pages)));
#ifdef ENABLE_MTK_MEMINFO
		g_mtk_gpu_total_memory_usage_in_pages_debugfs = atomic_read(&(kbdev->memdev.used_pages));
#endif /* ENABLE_MTK_MEMINFO */
		mutex_lock(&kbdev->kctx_list_lock);
#ifdef ENABLE_MTK_MEMINFO
		mtk_kbase_gpu_meminfo_index = 0;
#endif /* ENABLE_MTK_MEMINFO */
		list_for_each_entry(element, &kbdev->kctx_list, link) {
			/* output the memory usage and cap for each kctx
			* opened on this device */
			seq_printf(sfile, "  %s-0x%p %10u  %10u\n",
				"kctx",
				element->kctx,
				atomic_read(&(element->kctx->used_pages)),
				element->kctx->tgid);
#ifdef ENABLE_MTK_MEMINFO
			mtk_kbase_set_gpu_meminfo(mtk_kbase_gpu_meminfo_index, element->kctx->tgid,
				(int)atomic_read(&(element->kctx->used_pages)));
			mtk_kbase_gpu_meminfo_index++;
#endif /* ENABLE_MTK_MEMINFO */
		}
		mutex_unlock(&kbdev->kctx_list_lock);
	}
	kbase_dev_list_put(kbdev_list);
	return 0;
}

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER

/* fosmod_fireos_crash_reporting begin */
void lmk_add_to_buffer(const char *fmt, ...);

void kbasep_gpu_memory_seq_show_lmk(void)
{
	struct list_head *entry;
	const struct list_head *kbdev_list;

#ifdef ENABLE_MTK_MEMINFO
	ssize_t mtk_kbase_gpu_meminfo_index;

	mtk_kbase_reset_gpu_meminfo();
#endif /* ENABLE_MTK_MEMINFO */

	kbdev_list = kbase_dev_list_get();
	list_for_each(entry, kbdev_list) {
		struct kbase_device *kbdev = NULL;
		struct kbasep_kctx_list_element *element;

		kbdev = list_entry(entry, struct kbase_device, entry);
		/* output the total memory usage and cap for this device */
		lmk_add_to_buffer("%-25s  %-10s  %-10s  %-10s  %-10s\n"\
				"==============================================================================================================\n",
				"kbase_context", "pid", "gpu_mem", "sys_id", "tgid");

#ifdef ENABLE_MTK_MEMINFO
		g_mtk_gpu_total_memory_usage_in_pages_debugfs = atomic_read(&(kbdev->memdev.used_pages));
#endif /* ENABLE_MTK_MEMINFO */

		mutex_lock(&kbdev->kctx_list_lock);
#ifdef ENABLE_MTK_MEMINFO
		mtk_kbase_gpu_meminfo_index = 0;
#endif /* ENABLE_MTK_MEMINFO */

		list_for_each_entry(element, &kbdev->kctx_list, link) {
			/* output the memory usage and cap for each kctx
			* opened on this device */
			lmk_add_to_buffer("  %s-0x%p  %-10u  %-10u  %-10u  %-10u\n",
				"kctx",
				element->kctx, \
				element->kctx->pid,
				atomic_read(&(element->kctx->used_pages)) * PAGE_SIZE,
				element->kctx->id,
				element->kctx->tgid);

#ifdef ENABLE_MTK_MEMINFO
			mtk_kbase_set_gpu_meminfo(mtk_kbase_gpu_meminfo_index, element->kctx->tgid, \
					(int)atomic_read(&(element->kctx->used_pages)));
			mtk_kbase_gpu_meminfo_index++;
#endif /* ENABLE_MTK_MEMINFO */
		}
		lmk_add_to_buffer("%-16s (total gpu mem)  %10u bytes\n",
				kbdev->devname,
				atomic_read(&(kbdev->memdev.used_pages)) * PAGE_SIZE);
		mutex_unlock(&kbdev->kctx_list_lock);
	}
	kbase_dev_list_put(kbdev_list);
}
/* fosmod_fireos_crash_reporting begin */

#endif /* CONFIG_LOW_MEMORY_KILLER */

/*
 *  File operations related to debugfs entry for gpu_memory
 */
static int kbasep_gpu_memory_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, kbasep_gpu_memory_seq_show, NULL);
}

static const struct file_operations kbasep_gpu_memory_debugfs_fops = {
	.open = kbasep_gpu_memory_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*
 *  Initialize debugfs entry for gpu_memory
 */
void kbasep_gpu_memory_debugfs_init(struct kbase_device *kbdev)
{
	debugfs_create_file("gpu_memory", S_IRUGO,
			kbdev->mali_debugfs_directory, NULL,
			&kbasep_gpu_memory_debugfs_fops);
	return;
}

#else
/*
 * Stub functions for when debugfs is disabled
 */
void kbasep_gpu_memory_debugfs_init(struct kbase_device *kbdev)
{
	return;
}
#endif
