/*
 *  Copyright (C) 2008 Sven Köhler
 *
 *  This file is part of Nupkux.
 *
 *  Nupkux is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Nupkux is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Nupkux.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <task.h>
#include <lib/memory.h>
#include <fs/fs.h>

volatile task *current_task = 0;
volatile task tasks[NR_TASKS];

//paging.c
extern page_directory *kernel_directory;
extern page_directory *current_directory;

extern UINT read_eip(); //process.asm
extern UINT initial_esp; //defined in main.c

void move_stack(void *new_stack, UINT size)
{
	UINT i,pdirpos,old_esp,old_ebp,offset,new_esp,new_ebp,tmp;
	
	for (i=(UINT)new_stack;i>=((UINT)new_stack-size);i-=FRAME_SIZE) 
		make_page(i,PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | PAGE_FLAG_USERMODE,current_directory,1);
	asm volatile("mov %%cr3,%0":"=r"(pdirpos));
	asm volatile("mov %0,%%cr3"::"r"(pdirpos));
	asm volatile("mov %%esp,%0":"=r"(old_esp));
	asm volatile("mov %%ebp,%0":"=r"(old_ebp));
	offset=(UINT)new_stack-initial_esp;
	new_esp=old_esp+offset;
	new_ebp=old_ebp+offset;
	memcpy((void *)new_esp,(void*)old_esp,initial_esp-old_esp);
	for (i=(UINT)new_stack;i>(UINT)new_stack-size;i-=4) {
		tmp=*(UINT *)i;
		if ((old_esp<tmp) && (tmp<initial_esp))
			*((UINT *)i)=tmp+offset;
	}
	asm volatile("mov %0,%%esp"::"r"(new_esp));
	asm volatile("mov %0,%%ebp"::"r"(new_ebp));
}

void setup_tasking()
{
	cli();
	move_stack((void*)0xE0000000, 0x2000);
	UINT i;
	for (i=1;i<NR_TASKS;i++)
		tasks[i].pid=TASK_NOTASK;
	current_task=tasks;
	//Kernel has pid 0, stupid,
	//but init on Linux an other Unix-likes seems to have pid 1
	current_task->pid=0;
	current_task->parent=0;
	current_task->esp=current_task->ebp=0;
	current_task->eip=0;
	current_task->directory=current_directory;
	current_task->gid=current_task->uid=0; //root runs it
	current_task->pwd=0;//get_root_fs_node();
	sti();
}

void switch_task()
{
	if (!current_task) return;
	UINT esp, ebp, eip,i;
	asm volatile("mov %%esp,%0":"=r"(esp));
	asm volatile("mov %%ebp,%0":"=r"(ebp));
	eip=read_eip();
	if (eip==0x2DF) return; //Just switched
	current_task->eip=eip;
	current_task->esp=esp;
	current_task->ebp=ebp;
	
	i=current_task->pid;
	while (++i<NR_TASKS)
		if (tasks[i].pid!=TASK_NOTASK) break;
	if (i==NR_TASKS) current_task=tasks;
		else current_task=&(tasks[i]);
	eip=current_task->eip;
	esp=current_task->esp;
	ebp=current_task->ebp;
	current_directory=current_task->directory;
	asm volatile(	"cli\n\t"
			"mov %0,%%ecx\n\t"
			"mov %1,%%esp\n\t"
			"mov %2,%%ebp\n\t"
			"mov %3,%%cr3\n\t"
			"mov $0x2DF,%%eax\n\t"
			"sti\n\t"
			"jmp *%%ecx\n\t"::"r"(eip),"r"(esp),"r"(ebp),"r"(current_directory->physPos));
}

long fork()
{
	cli();
	task *parent_task=(task *)current_task;
	long i=0;
	while (++i<NR_TASKS)
		if (tasks[i].pid==TASK_NOTASK) break;
	if (i==NR_TASKS) return -1;
	page_directory *directory=clone_directory(current_directory);
	volatile task *newtask=(&(tasks[i]));
	newtask->pid=i;
	newtask->esp=0;
	newtask->ebp=0;
	newtask->eip=0;
	newtask->parent=parent_task->pid;
	newtask->gid=parent_task->gid;
	newtask->uid=parent_task->uid;
	newtask->directory=directory;
	newtask->pwd=parent_task->pwd;
		
	UINT eip=read_eip();
	if (current_task==parent_task) {
		UINT esp,ebp;
		asm volatile("mov %%esp,%0":"=r"(esp));
		asm volatile("mov %%ebp,%0":"=r"(ebp));
		newtask->esp=esp;
		newtask->ebp=ebp;
		newtask->eip=eip;
		sti();
		return newtask->pid;
	} else {
		sti();
		return 0;
	}
}

long getpid()
{
	return current_task->pid;
}
