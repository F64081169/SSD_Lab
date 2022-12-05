# LAB2: Implementing READ/WRITE through SCSI

## How to use?
```
make
```
- Read example
```
sudo ./scsi_test /dev/sg1 read lba10 s_cnt3
```
- Write example
```
sudo ./scsi_test /dev/sg1 write lba10 s_cnt1 ff
```