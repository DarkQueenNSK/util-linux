
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "c.h"
#include "fdisk.h"

struct menu_entry {
	const char	key;
	const char	*title;
	unsigned int	normal : 1,
			expert : 1,
			hidden : 1;

	enum fdisk_labeltype	exclude;
};

#define IS_MENU_SEP(e)	((e)->key == '-')
#define IS_MENU_HID(e)	((e)->hidden)

struct menu {
	enum fdisk_labeltype	label;		/* only for this label */
	enum fdisk_labeltype	exclude;	/* all labels except this */

	int (*callback)(struct fdisk_context *,
			const struct menu *,
			const struct menu_entry *);

	struct menu_entry	entries[];	/* NULL terminated array */
};

struct menu_context {
	size_t		menu_idx;		/* the current menu */
	size_t		entry_idx;		/* index with in the current menu */
};

#define MENU_CXT_EMPTY	{ 0, 0 }

/*
 * Menu entry macros:
 *	MENU_X*    expert mode only
 *      MENU_B*    both -- expert + normal mode
 *
 *      *_E exclude
 *      *_H hidden
 */

/* separator */
#define MENU_SEP(t)		{ .title = t, .key = '-', .normal = 1 }
#define MENU_XSEP(t)		{ .title = t, .key = '-', .expert = 1 }
#define MENU_BSEP(t)		{ .title = t, .key = '-', .expert = 1, .normal = 1 }

/* entry */
#define MENU_ENT(k, t)		{ .title = t, .key = k, .normal = 1 }
#define MENU_ENT_E(k, t, l)	{ .title = t, .key = k, .normal = 1, .exclude = l }

#define MENU_XENT(k, t)		{ .title = t, .key = k, .expert = 1 }
#define MENU_XENT_H(k, t)	{ .title = t, .key = k, .expert = 1, .hidden = 1 }

#define MENU_BENT(k, t)		{ .title = t, .key = k, .expert = 1, .normal = 1 }


/* Generic menu */
struct menu menu_generic = {
/*	.callback	= generic_menu_cb,*/
	.entries	= {
		MENU_BSEP(N_("Generic")),
		MENU_ENT  ('d', N_("delete a partition")),
		MENU_ENT  ('l', N_("list known partition types")),
		MENU_ENT  ('n', N_("add a new partition")),
		MENU_BENT ('p', N_("print the partition table")),
		MENU_ENT  ('t', N_("change a partition type")),
		MENU_ENT  ('v', N_("verify the partition table")),

		MENU_XENT('d', N_("print the raw data of the first sector")),

		MENU_SEP(N_("Misc")),
		MENU_BENT ('m', N_("print this menu")),
		MENU_ENT_E('u', N_("change display/entry units"), FDISK_DISKLABEL_GPT),
		MENU_ENT  ('x', N_("extra functionality (experts only)")),

		MENU_BSEP(N_("Save & Exit")),
		MENU_ENT_E('w', N_("write table to disk and exit"), FDISK_DISKLABEL_OSF),
		MENU_BENT ('q', N_("quit without saving changes")),
		MENU_XENT ('r', N_("return to main menu")),

		{ 0, NULL }
	}
};

struct menu menu_createlabel = {
/*	.callback = createlabel_menu_cb, */
	.exclude = FDISK_DISKLABEL_OSF,
	.entries = {
		MENU_SEP(N_("Create a new label")),
		MENU_ENT('g', N_("create a new empty GPT partition table")),
		MENU_ENT('G', N_("create a new empty SGI (IRIX) partition table")),
		MENU_ENT('o', N_("create a new empty DOS partition table")),
		MENU_ENT('s', N_("create a new empty Sun partition table")),

		/* backward compatibility -- be sensitive to 'g', but don't
		 * print it in the expert menu */
		MENU_XENT_H('g', N_("create an IRIX (SGI) partition table")),
		{ 0, NULL }
	}
};

struct menu menu_gpt = {
/*	.callback = gpt_menu_cb, */
	.label = FDISK_DISKLABEL_GPT,
	.entries = {
		MENU_XSEP(N_("GPT")),
		MENU_XENT('u', N_("change partition UUID")),
		MENU_XENT('n', N_("change partition name")),
		{ 0, NULL }
	}
};

struct menu menu_sun = {
/*	.callback = sun_menu_cb, */
	.label = FDISK_DISKLABEL_SUN,
	.entries = {
		MENU_BSEP(N_("Sun")),
		MENU_ENT('a', N_("toggle a read only flag")),
		MENU_ENT('c', N_("toggle the mountable flag")),

		MENU_XENT('a', N_("change number of alternate cylinders")),
		MENU_XENT('c', N_("change number of cylinders")),
		MENU_XENT('e', N_("change number of extra sectors per cylinder")),
		MENU_XENT('h', N_("change number of heads")),
		MENU_XENT('i', N_("change interleave factor")),
		MENU_XENT('o', N_("change rotation speed (rpm)")),
		MENU_XENT('s', N_("change number of sectors/track")),
		MENU_XENT('y', N_("change number of physical cylinders")),
		{ 0, NULL }
	}
};

struct menu menu_sgi = {
/*	.callback = sgi_menu_cb, */
	.label = FDISK_DISKLABEL_SGI,
	.entries = {
		MENU_SEP(N_("SGI")),
		MENU_ENT('a', N_("select bootable partition")),
		MENU_ENT('b', N_("edit bootfile entry")),
		MENU_ENT('c', N_("select sgi swap partition")),
		{ 0, NULL }
	}
};

struct menu menu_dos = {
/*	.callback = dos_menu_cb, */
	.label = FDISK_DISKLABEL_DOS,
	.entries = {
		MENU_BSEP(N_("DOS (MBR)")),
		MENU_ENT('a', N_("toggle a bootable flag")),
		MENU_ENT('b', N_("edit nested BSD disklabel")),
		MENU_ENT('c', N_("toggle the dos compatibility flag")),

		MENU_XENT('b', N_("move beginning of data in a partition")),
		MENU_XENT('c', N_("change number of cylinders")),		MENU_XENT('e', N_("list extended partitions")),
		MENU_XENT('f', N_("fix partition order")),
		MENU_XENT('h', N_("change number of heads")),
		MENU_XENT('i', N_("change the disk identifier")),
		MENU_XENT('s', N_("change number of sectors/track")),
		{ 0, NULL }
	}
};

struct menu menu_bsd = {
/*	.callback = bsd_menu_cb, */
	.label = FDISK_DISKLABEL_OSF,
	.entries = {
		MENU_SEP(N_("BSD")),
		MENU_ENT('e', N_("edit drive data")),
		MENU_ENT('i', N_("install bootstrap")),
		MENU_ENT('s', N_("show complete disklabel")),
		MENU_ENT('w', N_("write disklabel to disk")),
#if !defined (__alpha__)
		MENU_ENT('x', N_("link BSD partition to non-BSD partition")),
#endif
		{ 0, NULL }
	}
};

static const struct menu *menus[] = {
	&menu_gpt,
	&menu_sun,
	&menu_sgi,
	&menu_dos,
	&menu_bsd,
	&menu_generic,
	&menu_createlabel,
};

static const struct menu_entry *next_menu_entry(
			struct fdisk_context *cxt,
			struct menu_context *mc)
{
	while (mc->menu_idx < ARRAY_SIZE(menus)) {
		const struct menu *m = menus[mc->menu_idx];
		const struct menu_entry *e = &(m->entries[mc->entry_idx]);

		/* move to the next submenu if there is no more entries */
		if (e->title == NULL ||
		    (m->label && cxt->label && !(m->label & cxt->label->id))) {
			mc->menu_idx++;
			mc->entry_idx = 0;
			continue;
		}

		/* is the entry excluded for the current label? */
		if ((e->exclude && cxt->label &&
		     e->exclude & cxt->label->id) ||
		/* exclude non-expert entries in expect mode */
		    (e->expert == 0 && fdisk_context_display_details(cxt)) ||
		/* exclude non-normal entries in normal mode */
		    (e->normal == 0 && !fdisk_context_display_details(cxt))) {

			mc->entry_idx++;
			continue;
		}
		mc->entry_idx++;
		return e;

	}
	return NULL;
}

/* returns @menu and menu entry for then @key */
static const struct menu_entry *get_fdisk_menu_entry(
		struct fdisk_context *cxt,
		int key,
		const struct menu **menu)
{
	struct menu_context mc = MENU_CXT_EMPTY;
	const struct menu_entry *e;

	while ((e = next_menu_entry(cxt, &mc))) {
		if (IS_MENU_SEP(e) || e->key != key)
			continue;

		if (menu)
			*menu = menus[mc.menu_idx];
		return e;
	}

	return NULL;
}

static int menu_detect_collisions(struct fdisk_context *cxt)
{
	struct menu_context mc = MENU_CXT_EMPTY;
	const struct menu_entry *e, *r;

	while ((e = next_menu_entry(cxt, &mc))) {
		if (IS_MENU_SEP(e))
			continue;

		r = get_fdisk_menu_entry(cxt, e->key, NULL);
		if (!r) {
			DBG(CONTEXT, dbgprint("warning: not found "
					"entry for %c", e->key));
			return -1;
		}
		if (r != e) {
			DBG(CONTEXT, dbgprint("warning: duplicate key '%c'",
						e->key));
			DBG(CONTEXT, dbgprint("         %s", e->title));
			DBG(CONTEXT, dbgprint("         %s", r->title));
			abort();
		}
	}

	return 0;
}

int print_fdisk_menu(struct fdisk_context *cxt)
{
	struct menu_context mc = MENU_CXT_EMPTY;
	const struct menu_entry *e;

	ON_DBG(CONTEXT, menu_detect_collisions(cxt));

	if (fdisk_context_display_details(cxt))
		printf(_("\nHelp (expert commands):\n"));
	else
		printf(_("\nHelp:\n"));

	while ((e = next_menu_entry(cxt, &mc))) {
		if (IS_MENU_HID(e))
			continue;	/* hidden entry */
		if (IS_MENU_SEP(e))
			printf("\n  %s\n", _(e->title));
		else
			printf("   %c   %s\n", e->key, _(e->title));
	}
	fputc('\n', stdout);

	return 0;
}

/* Asks for command, verify the key and perform the command or
 * returns the command key if no callback for the command is
 * implemented.
 *
 * Returns: <0 on error
 *           0 on success (the command performed)
 *          >0 if no callback (then returns the key)
 */
int process_fdisk_menu(struct fdisk_context *cxt)
{
	const struct menu_entry *ent;
	const struct menu *menu;
	int key, rc;
	const char *prompt;
	char buf[BUFSIZ];

	if (fdisk_context_display_details(cxt))
		prompt = _("Expert command (m for help): ");
	else
		prompt = _("Command (m for help): ");

	fputc('\n',stdout);
	rc = get_user_reply(cxt, prompt, buf, sizeof(buf));
	if (rc)
		return rc;

	key = buf[0];
	ent = get_fdisk_menu_entry(cxt, key, &menu);
	if (!ent) {
		fdisk_warnx(cxt, _("%c: unknown command"), key);
		return -EINVAL;
	}

	DBG(CONTEXT, dbgprint("selected: key=%c, entry='%s'",
				key, ent->title));
	/* hardcoded help */
	if (key == 'm') {
		print_fdisk_menu(cxt);
		return 0;

	/* menu has implemented callback, use it */
	} else if (menu->callback)
		return menu->callback(cxt, menu, ent);

	/* no callback, return the key */
	return key;
}


#ifdef TEST_PROGRAM
struct fdisk_label *fdisk_new_dos_label(struct fdisk_context *cxt) { return NULL; }
struct fdisk_label *fdisk_new_bsd_label(struct fdisk_context *cxt) { return NULL; }
struct fdisk_label *fdisk_new_mac_label(struct fdisk_context *cxt) { return NULL; }
struct fdisk_label *fdisk_new_sgi_label(struct fdisk_context *cxt) { return NULL; }

int main(int argc, char *argv[])
{
	struct fdisk_context *cxt;
	int idx = 1;

	fdisk_init_debug(0);
	cxt = fdisk_new_context();

	if (argc > idx && strcmp(argv[idx], "--expert") == 0) {
		fdisk_context_enable_details(cxt, 1);
		idx++;
	}
	fdisk_context_switch_label(cxt, argc > idx ? argv[idx] : "gpt");

	print_fdisk_menu(cxt);
	return 0;
}
#endif
