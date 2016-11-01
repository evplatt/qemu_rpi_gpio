/*
 * Raspberry Pi 1 General Purpose IO with Passthrough to Host
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include <sys/shm.h>
#include <errno.h>
#include <string.h>

#define DEBUG_RPI_GPIO 1

#ifdef DEBUG_RPI_GPIO
#define DPRINTF(fmt, ...) \
do { printf("rpi_gpio: " fmt , ## __VA_ARGS__); } while (0)
#define BADF(fmt, ...) \
do { fprintf(stderr, "rpi_gpio: error: " fmt , ## __VA_ARGS__); exit(1);} while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#define BADF(fmt, ...) \
do { fprintf(stderr, "rpi_gpio: error: " fmt , ## __VA_ARGS__);} while (0)
#endif

#define TYPE_RPI_GPIO "rpi_gpio"
#define RPI_GPIO(obj) OBJECT_CHECK(RPI_GPIO_State, (obj), TYPE_RPI_GPIO)

typedef struct shared_gpio_state {

  uint32_t GPFSEL0;
  uint32_t GPFSEL1;
  uint32_t GPFSEL2;
  uint32_t GPFSEL3;
  uint32_t GPFSEL4;
  uint32_t GPFSEL5;
  uint32_t GPLEV0;
  uint32_t GPLEV1;
  uint32_t outstate0;
  uint32_t outstate1;

} shared_gpio_state;

typedef struct RPI_GPIO_State {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    uint32_t outstate0; /* Holds the output state for pins  0-31 */
    uint32_t outstate1; /* Holds the output state for pins 32-53 */
    uint32_t GPFSEL0;   /* 0x00 Function Select Pins 0-9   */
    uint32_t GPFSEL1;   /* 0x04 Function Select Pins 10-19 */
    uint32_t GPFSEL2;   /* 0x08 Function Select Pins 20-29 */
    uint32_t GPFSEL3;   /* 0x0c Function Select Pins 30-39 */
    uint32_t GPFSEL4;   /* 0x10 Function Select Pins 40-49 */
    uint32_t GPFSEL5;   /* 0x14 Function Select Pins 50-53 */
    uint32_t res0;      /* 0x18 */
    uint32_t GPSET0;    /* 0x1c Output Set Pins  0-31 [0=No Effect | 1=Set] */
    uint32_t GPSET1;    /* 0x20 Output Set Pins 32-53 [0=No Effect | 1=Set] */
    uint32_t res1;      /* 0x24 */
    uint32_t GPCLR0;    /* 0x28 Output Clear Pins   0-31 [0=No Effect | 1=Clear] */
    uint32_t GPCLR1;    /* 0x2c Output Clear Pins  32-53 [0=No Effect | 1=Clear] */
    uint32_t res2;      /* 0x30 */
    uint32_t GPLEV0;    /* 0x34 Pin Level (Read Only)  0-31 [0=Low | 1=High] */
    uint32_t GPLEV1;    /* 0x38 Pin Level (Read Only) 32-53 [0=Low | 1=High] */
    uint32_t res3;      /* 0x3c */
    uint32_t GPEDS0;    /* 0x40 Event Detect Pins  0-31 [0=Event Not Detected | 1=Event Detected] */
    uint32_t GPEDS1;    /* 0x44 Event Detect Pins 32-53 [0=Event Not Detected | 1=Event Detected] */
    uint32_t res4;      /* 0x48 */
    uint32_t GPREN0;    /* 0x4c Rising Edge Detect Enable Pins  0-31 [0=Rising Edge Detect Disabled | 1=Rising Edge Detect Enabled] */
    uint32_t GPREN1;    /* 0x50 Rising Edge Detect Enable Pins 32-53 [0=Rising Edge Detect Disabled | 1=Rising Edge Detect Enabled] */
    uint32_t res5;      /* 0x54 */
    uint32_t GPFEN0;    /* 0x58 Falling Edge Detect Enable Pins  0-31 [0=Falling Edge Detect Disabled | 1=Falling Edge Detect Enabled] */
    uint32_t GPFEN1;    /* 0x5c Falling Edge Detect Enable Pins 32-53 [0=Falling Edge Detect Disabled | 1=Falling Edge Detect Enabled] */
    uint32_t res6;      /* 0x60 */
    uint32_t GPHEN0;    /* 0x64 High Level Detect Enable Bits  0-31 [0=High Level Detect Disabled | 1=High Level Detect Enabled] */
    uint32_t GPHEN1;    /* 0x68 High Level Detect Enable Bits 32-53 [0=High Level Detect Disabled | 1=High Level Detect Enabled] */
    uint32_t res7;      /* 0x6c */
    uint32_t GPLEN0;    /* 0x70 Low Level Detect Enable Bits  0-31 [0=Low Level Detect Disabled | 1=Low Level Detect Enabled] */
    uint32_t GPLEN1;    /* 0x74 Low Level Detect Enable Bits 32-53 [0=Low Level Detect Disabled | 1=Low Level Detect Enabled] */
    uint32_t res8;      /* 0x78 */
    uint32_t GPAREN0;   /* 0x7c Async Rising Edge Detect Enable Pins  0-31 [1=Async Rising Edge Detect Disabled | 1=Async Rising Edge Detect Enabled] */
    uint32_t GPAREN1;   /* 0x80 Async Rising Edge Detect Enable Pins 32-53 [1=Async Rising Edge Detect Disabled | 1=Async Rising Edge Detect Enabled] */
    uint32_t res9;      /* 0x84 */
    uint32_t GPAFEN0;   /* 0x88 Async Falling Edge Detect Enable Pins  0-31 [1=Async Falling Edge Detect Disabled | 1=Async Falling Edge Detect Enabled] */
    uint32_t GPAFEN1;   /* 0x8c Async Falling Edge Detect Enable Pins 32-53 [1=Async Falling Edge Detect Disabled | 1=Async Falling Edge Detect Enabled] */
    uint32_t res10;     /* 0x90 */
    uint32_t GPPUD;     /* 0x94 Pull-Up/Pull-Down All Pins [Bits 1-0: 0=Disable Pull-Up/Pull-Down | 1=Enable Pull-Down Control | 2=Enable Pull-Up Control] */
    uint32_t GPPUDCLK0; /* 0x98 Pull-Up/Pull-Down Clock Pins  0-31 [0=No Effect | 1=Assert Clock on Line] */
    uint32_t GPPUDCLK1; /* 0x9c Pull-Up/Pull-Down Clock Pins 32-53 [0=No Effect | 1=Assert Clock on Line] */
    uint32_t res11;     /* 0xa0 */
    uint32_t test;      /* 0xb0 */
    qemu_irq out[54];   /* qdev currently wants an interrupt line for every output.  BCM2835 only has 3 multiplexed lines.  Let's pretend it's 54 for now. */
    shared_gpio_state *shm;  /* pointer to shared struct */
    const unsigned char *id;
} RPI_GPIO_State;

static const VMStateDescription vmstate_rpi_gpio = {
    .name = "rpi_gpio",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
      VMSTATE_UINT32(GPFSEL0, RPI_GPIO_State),
      VMSTATE_UINT32(GPFSEL1, RPI_GPIO_State),
      VMSTATE_UINT32(GPFSEL2, RPI_GPIO_State),
      VMSTATE_UINT32(GPFSEL3, RPI_GPIO_State),
      VMSTATE_UINT32(GPFSEL4, RPI_GPIO_State),
      VMSTATE_UINT32(GPFSEL5, RPI_GPIO_State),
      VMSTATE_UINT32(GPSET0, RPI_GPIO_State),
      VMSTATE_UINT32(GPSET1, RPI_GPIO_State),
      VMSTATE_UINT32(GPCLR0, RPI_GPIO_State),
      VMSTATE_UINT32(GPCLR1, RPI_GPIO_State),
      VMSTATE_UINT32(GPLEV0, RPI_GPIO_State),
      VMSTATE_UINT32(GPLEV1, RPI_GPIO_State),
      VMSTATE_UINT32(GPEDS0, RPI_GPIO_State),
      VMSTATE_UINT32(GPEDS1, RPI_GPIO_State),
      VMSTATE_UINT32(GPREN0, RPI_GPIO_State),
      VMSTATE_UINT32(GPREN1, RPI_GPIO_State),
      VMSTATE_UINT32(GPFEN0, RPI_GPIO_State),
      VMSTATE_UINT32(GPFEN1, RPI_GPIO_State),
      VMSTATE_UINT32(GPHEN0, RPI_GPIO_State),
      VMSTATE_UINT32(GPHEN1, RPI_GPIO_State),
      VMSTATE_UINT32(GPLEN0, RPI_GPIO_State),
      VMSTATE_UINT32(GPLEN1, RPI_GPIO_State),
      VMSTATE_UINT32(GPAREN0, RPI_GPIO_State),
      VMSTATE_UINT32(GPAREN1, RPI_GPIO_State),
      VMSTATE_UINT32(GPAFEN0, RPI_GPIO_State),
      VMSTATE_UINT32(GPAFEN1, RPI_GPIO_State),
      VMSTATE_UINT32(GPPUD, RPI_GPIO_State),
      VMSTATE_UINT32(GPPUDCLK0, RPI_GPIO_State),
      VMSTATE_UINT32(GPPUDCLK1, RPI_GPIO_State),
      VMSTATE_END_OF_LIST()
    }
};

static uint32_t get_pin_function(RPI_GPIO_State *s, int pin){

  if (pin<10)             return ((s->GPFSEL0 >> (3*pin))      & 0x7);
  if (pin>=10 && pin<20) return ((s->GPFSEL1 >> (3*(pin-10))) & 0x7);
  if (pin>=20 && pin<30) return ((s->GPFSEL2 >> (3*(pin-20))) & 0x7);
  if (pin>=30 && pin<40) return ((s->GPFSEL3 >> (3*(pin-30))) & 0x7);
  if (pin>=40 && pin<50) return ((s->GPFSEL4 >> (3*(pin-40))) & 0x7);
  return ((s->GPFSEL5 >> (3*(pin-50))) & 0x7);

}

static void rpi_gpio_update(RPI_GPIO_State *s)
{
  int i;
  uint32_t mask;

  for (i=0; i<32; i++){
    mask = (1 << i);
    if (get_pin_function(s,i) == 1){    /* make sure its an output */
      if (s->GPSET0 & mask){          /* set flag is set */
        s->outstate0 |= mask;            /* set the state bit */
        s->GPSET0 &= ~mask;           /* clear the set bit */
      }
      else if (s->GPCLR0 & mask){     /* clear flag is set */
        s->outstate0 &= ~mask;            /* set the state bit */
        s->GPCLR0 &= ~mask;           /* clear the set bit */
      }
    }
  }
  for (i=32; i<54; i++){
    mask = (1 << (i-32));
    if (get_pin_function(s,i) == 1){    /* make sure its an output */
      if (s->GPSET1 & mask){          /* set flag is set */
        s->outstate1 |= mask;            /* set the state bit */
        s->GPSET1 &= ~mask;           /* clear the set bit */
      }
      else if (s->GPCLR1 & mask){     /* clear flag is set */
        s->outstate1 &= ~mask;            /* set the state bit */
        s->GPCLR1 &= ~mask;           /* clear the set bit */
      }
    }
  }

  s->shm->GPFSEL0 = s->GPFSEL0;
  s->shm->GPFSEL1 = s->GPFSEL1;
  s->shm->GPFSEL2 = s->GPFSEL2;
  s->shm->GPFSEL3 = s->GPFSEL3;
  s->shm->GPFSEL4 = s->GPFSEL4;
  s->shm->GPFSEL5 = s->GPFSEL5;
  s->shm->outstate0 = s->outstate0;
  s->shm->outstate1 = s->outstate1;

}

static uint64_t rpi_gpio_read(void *opaque, hwaddr offset,
                          unsigned size)
{
    RPI_GPIO_State *s = (RPI_GPIO_State *)opaque;

    DPRINTF("rpi_gpio_debug:  rpi_gpio_read - offset=%02x, size=%d\n", (int)offset, size);

    switch (offset) {
      case 0x00:
          return s->GPFSEL0;
      case 0x04:
          return s->GPFSEL1;
      case 0x08:
          return s->GPFSEL2;
      case 0x0c:
          return s->GPFSEL3;
      case 0x10:
          return s->GPFSEL4;
      case 0x14:
          return s->GPFSEL5;
      case 0x34:
          return s->GPLEV0;
      case 0x38:
          return s->GPLEV1;
      case 0x40:
          return s->GPEDS0;
      case 0x44:
          return s->GPEDS1;
      case 0x4c:
          return s->GPREN0;
      case 0x50:
          return s->GPREN1;
      case 0x58:
          return s->GPFEN0;
      case 0x5c:
          return s->GPFEN1;
      case 0x64:
          return s->GPHEN0;
      case 0x68:
          return s->GPHEN1;
      case 0x70:
          return s->GPLEN0;
      case 0x74:
          return s->GPLEN1;
      case 0x7c:
          return s->GPAREN0;
      case 0x80:
          return s->GPAREN1;
      case 0x88:
          return s->GPAFEN0;
      case 0x8c:
          return s->GPAFEN1;
      case 0x94:
          return s->GPPUD;
      case 0x98:
          return s->GPPUDCLK0;
      case 0x9c:
          return s->GPPUDCLK1;
      case 0x1c: /* GPSET0 (Write-Only) */
      case 0x20: /* GPSET1 (Write-Only) */
      case 0x28: /* GPCLR0 (Write-Only) */
      case 0x2c: /* GPCLR1 (Write-Only) */
      case 0x18: /* res0 */
      case 0x24: /* res1 */
      case 0x30: /* res2 */
      case 0x3c: /* res3 */
      case 0x48: /* res4 */
      case 0x54: /* res5 */
      case 0x60: /* res6 */
      case 0x6c: /* res7 */
      case 0x78: /* res8 */
      case 0x84: /* res9 */
      case 0x90: /* res10 */
      case 0xa0: /* res11 */
      case 0xb0: /* test */
          goto err_out;
      default:
          break;
    }
err_out:
    qemu_log_mask(LOG_GUEST_ERROR,
                  "rpi_gpio_read: Bad offset %x\n", (int)offset);
    return 0;
}

static void rpi_gpio_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    RPI_GPIO_State *s = (RPI_GPIO_State *)opaque;

    DPRINTF("rpi_gpio_write - offset=%02x, size=%d, value=%d\n", (int)offset, size, (uint32_t)value);

    switch (offset) {
      case 0x00:
          s->GPFSEL0 = (value & 0xffffffff);
          break;
      case 0x04:
          s->GPFSEL1 = (value & 0xffffffff);
          break;
      case 0x08:
          s->GPFSEL2 = (value & 0xffffffff);
          break;
      case 0x0c:
          s->GPFSEL3 = (value & 0xffffffff);
          break;
      case 0x10:
          s->GPFSEL4 = (value & 0xffffffff);
          break;
      case 0x14:
          s->GPFSEL5 = (value & 0xffffffff);
          break;
      case 0x1c:
          s->GPSET0 = (value & 0xffffffff);
          break;
      case 0x20:
          s->GPSET1 = (value & 0xffffffff);
          break;
      case 0x28:
          s->GPCLR0 = (value & 0xffffffff);
          break;
      case 0x2c:
          s->GPCLR1 = (value & 0xffffffff);
          break;
      case 0x40:
          s->GPEDS0 = (value & 0xffffffff);
          break;
      case 0x44:
          s->GPEDS1 = (value & 0xffffffff);
          break;
      case 0x4c:
          s->GPREN0 = (value & 0xffffffff);
          break;
      case 0x50:
          s->GPREN1 = (value & 0xffffffff);
          break;
      case 0x58:
          s->GPFEN0 = (value & 0xffffffff);
          break;
      case 0x5c:
          s->GPFEN1 = (value & 0xffffffff);
          break;
      case 0x64:
          s->GPHEN0 = (value & 0xffffffff);
          break;
      case 0x68:
          s->GPHEN1 = (value & 0xffffffff);
          break;
      case 0x70:
          s->GPLEN0 = (value & 0xffffffff);
          break;
      case 0x74:
          s->GPLEN1 = (value & 0xffffffff);
          break;
      case 0x7c:
          s->GPAREN0 = (value & 0xffffffff);
          break;
      case 0x80:
          s->GPAREN1 = (value & 0xffffffff);
          break;
      case 0x88:
          s->GPAFEN0 = (value & 0xffffffff);
          break;
      case 0x8c:
          s->GPAFEN1 = (value & 0xffffffff);
          break;
      case 0x94:
          s->GPPUD = (value & 0xffffffff);
          break;
      case 0x98:
          s->GPPUDCLK0 = (value & 0xffffffff);
          break;
      case 0x9c:
          s->GPPUDCLK1 = (value & 0xffffffff);
          break;
      case 0x34: /* GPLEV0 (Read-Only) */
      case 0x38: /* GPLEV1 (Read-Only) */
      case 0x18: /* res0 */
      case 0x24: /* res1 */
      case 0x30: /* res2 */
      case 0x3c: /* res3 */
      case 0x48: /* res4 */
      case 0x54: /* res5 */
      case 0x60: /* res6 */
      case 0x6c: /* res7 */
      case 0x78: /* res8 */
      case 0x84: /* res9 */
      case 0x90: /* res10 */
      case 0xa0: /* res11 */
      case 0xb0: /* test */
      default:
          goto err_out;
    }
    rpi_gpio_update(s);
    return;
err_out:
    qemu_log_mask(LOG_GUEST_ERROR,
                  "rpi_gpio_write: Bad offset %x\n", (int)offset);
}

static void rpi_gpio_reset(DeviceState *dev)
{
    RPI_GPIO_State *s = RPI_GPIO(dev);

    s->GPFSEL0   = 0;
    s->GPFSEL1   = 0;
    s->GPFSEL2   = 0;
    s->GPFSEL3   = 0;
    s->GPFSEL4   = 0;
    s->GPFSEL5   = 0;
    s->GPSET0    = 0;
    s->GPSET1    = 0;
    s->GPCLR0    = 0;
    s->GPCLR1    = 0;
    s->GPLEV0    = 0;
    s->GPLEV1    = 0;
    s->GPEDS0    = 0;
    s->GPEDS1    = 0;
    s->GPREN0    = 0;
    s->GPREN1    = 0;
    s->GPFEN0    = 0;
    s->GPFEN1    = 0;
    s->GPHEN0    = 0;
    s->GPHEN1    = 0;
    s->GPLEN0    = 0;
    s->GPLEN1    = 0;
    s->GPAREN0   = 0;
    s->GPAREN1   = 0;
    s->GPAFEN0   = 0;
    s->GPAFEN1   = 0;
    s->GPPUD     = 0;
    s->GPPUDCLK0 = 0;
    s->GPPUDCLK1 = 0;

}

static void rpi_gpio_set(void * opaque, int line, int level)
{
  RPI_GPIO_State *s = (RPI_GPIO_State *)opaque;

  /* Since the function select registers aren't aligned with the level
   * registers, we need to check first to make sure this line is an input
   */
  if (get_pin_function(s,line) == 0){

    /* We have an input; Set the level */
    if (line<32){
      uint32_t mask = 1 << line;
      s->GPLEV0 &= ~mask;
      if (level) s->GPLEV0 |= mask;
    }
    else{
      uint32_t mask = 1 << (line-32);
      s->GPLEV1 &= ~mask;
      if (level) s->GPLEV1 |= mask;
    }
  }
}

static const MemoryRegionOps rpi_gpio_ops = {
    .read = rpi_gpio_read,
    .write = rpi_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static shared_gpio_state *get_shared_ptr(void);
static shared_gpio_state *get_shared_ptr(void){

  key_t key;
  int shmid=-1;

  key = ftok("/proc/cpuinfo",0x84);
  shmid = shmget(key, sizeof(shared_gpio_state), 0666 | IPC_CREAT);

  if (shmid != -1){
    DPRINTF("Created shared memory segment for rpi_gpio state\n");
  }
  else{
    DPRINTF("Failed to create shared memory segment. Error: %s\n", strerror(errno));
  }

  return shmat(shmid, (void *)0, 0);

}

static int rpi_gpio_initfn(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    RPI_GPIO_State *s = RPI_GPIO(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &rpi_gpio_ops, s,
                          "rpi_gpio", 0x00B1);
    sysbus_init_mmio(sbd, &s->iomem);
    qdev_init_gpio_in(dev, rpi_gpio_set, 54);
    qdev_init_gpio_out(dev, s->out, 54);

    s->shm = get_shared_ptr();

    return 0;
}

static void rpi_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = rpi_gpio_initfn;
    dc->vmsd = &vmstate_rpi_gpio;
    dc->reset = &rpi_gpio_reset;
}

static void rpi_gpio_init(Object *obj)
{
  DPRINTF("Initialized RPI_GPIO device\n");
}

static const TypeInfo rpi_gpio_info = {
    .name          = TYPE_RPI_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RPI_GPIO_State),
    .instance_init = rpi_gpio_init,
    .class_init    = rpi_gpio_class_init,
};

static void rpi_gpio_register_types(void)
{
    type_register_static(&rpi_gpio_info);
}

type_init(rpi_gpio_register_types)
