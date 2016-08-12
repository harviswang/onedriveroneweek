/*
 * KLD Skeleton (FreeBSD)
 * compile: make
 * install: sudo kldload ./skeleton.ko
 * uninstall: sudo kldunload skeleton
 */
#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h> /* uprintf */
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/kernel.h>

/*
 * Load handler that deals with the loading and unloading of a KLD.
 */
static int skel_loader(struct module *m, int what, void *arg)
{
	int err = 0;

	switch (what) {
	case MOD_LOAD:
		uprintf("Skeleton KLD loaded.\n");
		break;
	case MOD_UNLOAD:
		uprintf("Skeleton KLD unloaded.\n");
		break;
	default:
		err = EOPNOTSUPP;
		break;
	}
	return(err);
}

/* Declare this module to the rest of the kernel */

static moduledata_t skel_mod = {
	"skel",
	skel_loader,
	NULL
};

DECLARE_MODULE(skeleton, skel_mod, SI_SUB_KLD, SI_ORDER_ANY);
