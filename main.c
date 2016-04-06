/*
 * CS492 development module
 * Authors: 
 *  Daniel Berliner dberli2@illinois.edu
 *
 * Description:
 *  This is a test module for the team to do memory tests on.
 *  The final product will be based on code designed here.
 */
#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>  /* uprintf */
#include <sys/param.h>  /* defines used in kernel.h */
#include <sys/kernel.h> /* types used in module initialization */
#include <sys/conf.h>   /* cdevsw struct */
#include <sys/uio.h>    /* uio struct */
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/vmmeter.h>
#include <vm/vm.h>
#include <vm/swap_pager.h>
#include <vm/vm_param.h>
#include <vm/vm_page.h>
#include <vm/vm_phys.h>
#define BUFFERSIZE 255

extern int swap_pager_avail;
/* Function prototypes */
static d_open_t      lowmem_open;
static d_close_t     lowmem_close;
static d_read_t      lowmem_read;
static d_write_t     lowmem_write;

/* Character device entry points */
static struct cdevsw severe_cdevsw = {
  .d_version = D_VERSION,
  .d_open = lowmem_open,
  .d_close = lowmem_close,
  .d_read = lowmem_read,
  .d_write = lowmem_write,
  .d_name = "lowmem",
};


/* vars */
static struct cdev *severe_dev;
static const size_t PAYLOAD_LEN=5;
static char payload[PAYLOAD_LEN + sizeof(int)*2];

MALLOC_DECLARE(M_LOWMEMBUF);
MALLOC_DEFINE(M_LOWMEMBUF, "lowmembuffer", "buffer for lowmem module");



/*
 * This function is called by the kld[un]load(2) system calls to
 * determine what actions to take when a module is loaded or unloaded.
 */
static int
lowmem_loader(struct module *m __unused, int what, void *arg __unused)
{
  int error = 0;

  switch (what) {
  case MOD_LOAD:                /* kldload */
    error = make_dev_p(MAKEDEV_CHECKNAME | MAKEDEV_WAITOK,
        &severe_dev,
        &severe_cdevsw,
        0,
        UID_ROOT,
        GID_WHEEL,
        0600,
        "lowmem");
    if (error != 0)
      break;
    break;
  case MOD_UNLOAD:
    destroy_dev(severe_dev);
    break;
  default:
    error = EOPNOTSUPP;
    break;
  }
  return (error);
}

static int
lowmem_open(struct cdev *dev __unused, int oflags __unused, int devtype __unused,
    struct thread *td __unused)
{
  int error = 0;

  return (error);
}

static int
lowmem_close(struct cdev *dev __unused, int fflag __unused, int devtype __unused,
    struct thread *td __unused)
{
  return (0);
}

/*
 * The total virtual memory = phys memory available for paging plus the  amount of swap space
*/


/*
 * The read function just takes the buf that was saved via
 * lowmem_write() and returns it to userland for accessing.
 * uio(9)
 */
static int
lowmem_read(struct cdev *dev __unused, struct uio *uio, int ioflag __unused)
{
  int error = 0;
  size_t amt=PAYLOAD_LEN;
  //Assign the status bytes
  payload[0] = 0b0;
  if(vm_page_count_target())
    payload[0] |= 0b1;

  if(vm_page_count_min())
    payload[0] |= 0b10;
  
  if(vm_paging_target() > 0)
    payload[0] |= 0b100;

  if(vm_page_count_severe())
    payload[0] |= 0b1000;
  memcpy(&payload[1], &swap_pager_avail, sizeof(int));
  amt=MIN(uio->uio_resid, uio->uio_offset >= amt + 1 ? 0 :
      amt + 1 - uio->uio_offset);
	if ((error = uiomove(payload, amt, uio)) != 0){}
  return (error);
}

/*
 * lowmem_write takes in a character string and saves it
 * to buf for later accessing.
 */
static int
lowmem_write(struct cdev *dev __unused, struct uio *uio, int ioflag __unused)
{
  return 0;
}

DEV_MODULE(lowmem, lowmem_loader, NULL);
