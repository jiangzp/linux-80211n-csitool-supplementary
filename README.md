These modifications are made by Zhiping Jiang for our series experiments on CSITool. Most of them are done during 2012/2013.

Most of our experiments were carried out in Monitor + injection mode, and even injecting + logging simultaneously.

A problem we met was that we need to track the packets number and packets source (tx mac id) so as we could sync the tx/rx packets and  understand the packets lost in different channels and modulations.

To enable packet tracking, I made modifications to the kernel, random_packet, and matlab codes. 

The first step to send an incremental packet_count and mac_id numbers by the random_packets. I made some tricks to encapsulate these two numbers into the frame structure. 

On receiver side, the kernel is modified to extract these two numbers and write down it to the log file. 

To extract them in Matlab, an slightly customized read_bfee.c and a higher level file extractor extractCSIData(filename) Matlab function are presented.


--------------
injectionACK.c    invoke random_packets once received an CSI packet, the reply frame has the same frame number.

scripts:

buildKernel.sh  automatically build kernel, install it.
csi_prepare.sh  switch to monitor+ injection mode, it has three parameters. It requires aircrack-ng installed.
inj_prepare.sh  the same as csi_prepare.sh but in case of not installing aircrack-ng.



