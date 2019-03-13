```
crash> epython /cores/crashext/epython/storage/scsishow.py --time | grep active
scsi_cmnd ffffa0fd1f9a7b80 on scsi_device 0xffffa0f315ddd000 (1:0:1:92) is active, deadline: 39352 cmnd-alloc: -260648 rq-alloc: -260648

crash> scsi_cmnd.cmnd ffffa0fd1f9a7b80
  cmnd = 0xffffa0f3392cac98 "\022\001\200"

crash> rd -8 0xffffa0f3392cac98 6
ffffa0f3392cac98:  12 01 80 00 ff 00    
                   ^^      

 *      SCSI opcodes
 */

#define TEST_UNIT_READY       0x00
#define REZERO_UNIT           0x01
#define REQUEST_SENSE         0x03
#define FORMAT_UNIT           0x04
#define READ_BLOCK_LIMITS     0x05
#define REASSIGN_BLOCKS       0x07
#define INITIALIZE_ELEMENT_STATUS 0x07
#define READ_6                0x08
#define WRITE_6               0x0a
#define SEEK_6                0x0b
#define READ_REVERSE          0x0f
#define WRITE_FILEMARKS       0x10
#define SPACE                 0x11
#define INQUIRY               0x12   <=====
#define RECOVER_BUFFERED_DATA 0x14
#define MODE_SELECT           0x15
#define RESERVE               0x16

Was an inquiry command.
```
