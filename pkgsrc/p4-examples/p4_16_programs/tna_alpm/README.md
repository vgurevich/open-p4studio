## Summary

* Name: tna_alpm
* P4 version: P4_16
* Architectures: Tofino Native Architecture (TNA), Tofino2 Native Architecture (T2NA)
* Programming stack: Barefoot Runtime Interface (BRI)

Longest Prefix Matching (LPM) using algorithmin TCAM is essential for IP routing. It matches an IP address
with the routing table entry with the longest prefix mask.

This example demonstrates how to use the following ALPM (Algorithmic LPM) features to improve IP routing scale -
 - ATCAM subset key width optimization
 - ATCAM exclude MSB bits optimization
