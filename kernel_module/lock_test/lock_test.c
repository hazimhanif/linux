#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/hugetlb.h>  
#include <linux/rcupdate.h>
#include <linux/swap.h> 
#include <linux/mmdebug.h>
#include <linux/page-flags.h>
#include <linux/delay.h>
#include <linux/string.h>

MODULE_AUTHOR("Hazim Hanif");
MODULE_DESCRIPTION("Testing lock function");	
MODULE_LICENSE("GPL v2");

static char *proc_name="mysqld"; //Process name of the desired process that needs locking.
static struct task_struct *task;

static int do_lock(int flags)
{
    int nr_pages;
    struct vm_area_struct * vma;
    int lock=0;

    if (flags & MCL_FUTURE)
        task->mm->def_flags |= VM_LOCKED;
    else
        task->mm->def_flags &= ~VM_LOCKED;
    if (flags == MCL_FUTURE)
        goto out;

    for (vma = task->mm->mmap; vma ; vma = vma ->vm_next)
    {
        vm_flags_t newflags;

        newflags = vma->vm_flags | VM_LOCKED;
        if (!(flags & MCL_CURRENT))
            newflags &= ~VM_LOCKED;

        //Fixup part started.
        
        lock = !!(newflags & VM_LOCKED);


        nr_pages = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;

        if (!lock)
            nr_pages = -nr_pages;

        task->mm->locked_vm += nr_pages;
        task->mm->pinned_vm += nr_pages;
        vma->vm_flags = newflags;


        //Fixup part ended.

        cond_resched_rcu_qs();

    }

    printk("VM Total:%lu ,VM Locked:%lu, Pinned VM:%lu\n",task->mm->total_vm,task->mm->locked_vm,task->mm->pinned_vm);
    printk("VM_LOCKED:%d\n",VM_LOCKED);
    printk("DEFFLASGS:%lu\n",task->mm->def_flags);

out:
    return 0;

}

static int lock_test(int flags)
{

    unsigned long lock_limit;
    int ret = -EINVAL;

    printk("Start locking process:%s\n",proc_name);

    //Preparation for do_lock() function. Follows as in /linux/mm/mlock.c

    if (!flags || (flags & ~(MCL_CURRENT | MCL_FUTURE)))
        goto out;
 
    ret = -EPERM;
    if (!can_do_mlock())
        goto out;

    lock_limit = rlimit(RLIMIT_MEMLOCK);
    lock_limit >>= PAGE_SHIFT;

    ret = -ENOMEM;
    down_write(&task->mm->mmap_sem);

    if (!(flags & MCL_CURRENT) || (task->mm->total_vm <= lock_limit) || capable(CAP_IPC_LOCK))
        do_lock(flags);

    printk("Locking done!\n");

    up_write(&task->mm->mmap_sem);
    if (!ret && (flags & MCL_CURRENT))
        mm_populate(0, TASK_SIZE);

out:
    return ret;
}

static int unlock_test(int flags)
{
    int ret;

    printk("Unlocking process:%s\n",proc_name); 
    down_write(&task->mm->mmap_sem);

    //Call the locking function instead with input flags=0. This is followed as in munlockall defined in /linux/mm/mlock.c
    ret = do_lock(flags);
    up_write(&task->mm->mmap_sem);

    printk("Unlocking done!\n");
    return ret;

}


static int __init lock_test_init(void)
{   
    int found_flags=0;
    struct task_struct *p_in;
    printk("Load module: lock_test\n");

lookup:
    
    p_in=task;

    for_each_process(p_in)
    {   
        if(strcmp(proc_name,p_in->comm) ==0)
        {   
            found_flags=1;
            task=p_in;
            printk("Process name:%s, PID:%d\n",task->comm,task->pid);
            printk("Start area:0x%lx , End area:0x%lx\n",task->mm->mmap->vm_start,task->mm->mmap->vm_end);
            printk("VM Total:%lu ,VM Locked:%lu, Pinned VM:%lu\n",task->mm->total_vm,task->mm->locked_vm,task->mm->pinned_vm);
            printk("MCL_FUTURE:%d\n",MCL_FUTURE);
            printk("MCL_CURRENT:%d\n",MCL_CURRENT);
            printk("VM_LOCKED:%d\n",VM_LOCKED);
            printk("DEFFLASGS:%lu\n",task->mm->def_flags);
            printk("Vm flags:%lu\n",task->mm->mmap->vm_flags);

            //Call the locking function with input flags=3 as in mlockall(3). Lock current & future pages(stack,codes,everything..)
            lock_test(3);
            
        }
    }
    
    if(found_flags==0)
    {
        msleep_interruptible(10000);  // A delay of 10000ms is needed to avoid PID confusion in the loop during creation of new process.
        goto lookup;
    }

    return 0;

}

static void __exit lock_test_exit(void)
{   

    struct task_struct *p_out;

    for_each_process(p_out)
    {   
        if(strcmp(proc_name,p_out->comm) ==0)
        {
            task=p_out;
            unlock_test(0);
        }
    }
    

	printk("Unload module: lock_test. Goodbye!\n");
}


module_init(lock_test_init);
module_exit(lock_test_exit);
