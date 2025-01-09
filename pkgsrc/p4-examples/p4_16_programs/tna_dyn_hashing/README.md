## Summary

* Name: tna_dyn_hashing
* P4 version: P4_16
* Architectures: Tofino Native Architecture (TNA), Tofino2 Native Architecture (T2NA)
* Programming stack: Barefoot Runtime Interface (BRI)

The dynamic hashing feature allows the control plane to change the parameters
of a hash extern at runtime. The example demonstrates how this is done.
It demonstrates the following features

1. Changing order of hash fields when calculating hash
2. Masking portions of fields using field slicing
3. Setting fields to be symmetrically hashed during runtime
4. Changing algorithm to predefined/user-defined algorithms
5. Compute hash value with current hash configuration
6. Get ActionSelector member using hash value during runtime
