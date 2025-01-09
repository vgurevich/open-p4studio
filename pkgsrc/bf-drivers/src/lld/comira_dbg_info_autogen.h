reg_decoder_fld_t interrupts0_setintenable0_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10001,
     "Fifoctrl Ch#0 RX sync Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10002,
     "Fifoctrl Ch#1 RX sync Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10003,
     "Fifoctrl Ch#2 RX sync Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10004,
     "Fifoctrl Ch#3 RX sync Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10005,
     "Mac Ch#0 RX fault Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10006,
     "Mac Ch#1 RX fault Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10007,
     "Mac Ch#2 RX fault Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10008,
     "Mac Ch#3 RX fault Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10009,
     "SerDes Mux RX SigOk Ln#0 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10010,
     "SerDes Mux RX SigOk Ln#1 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten10",
     10,
     10,
     0,
     10011,
     "SerDes Mux RX SigOk Ln#2 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10012,
     "SerDes Mux RX SigOk Ln#3 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten12",
     12,
     12,
     0,
     10013,
     "MAC Ch#0 TX Idle Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten13",
     13,
     13,
     0,
     10014,
     "MAC Ch#1 TX Idle Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten14",
     14,
     14,
     0,
     10015,
     "MAC Ch#2 TX Idle Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten15",
     15,
     15,
     0,
     10016,
     "MAC Ch#3 TX Idle Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable0_flds = {
    16, interrupts0_setintenable0_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable0_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10017,
     "Fifoctrl Ch#0 RX sync Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10018,
     "Fifoctrl Ch#1 RX sync Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10019,
     "Fifoctrl Ch#2 RX sync Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10020,
     "Fifoctrl Ch#3 RX sync Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10021,
     "Mac Ch#0 RX fault Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10022,
     "Mac Ch#1 RX fault Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10023,
     "Mac Ch#2 RX fault Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10024,
     "Mac Ch#3 RX fault Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10025,
     "SerDes Mux RX SigOk Ln#0 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10026,
     "SerDes Mux RX SigOk Ln#1 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten10",
     10,
     10,
     0,
     10027,
     "SerDes Mux RX SigOk Ln#2 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10028,
     "SerDes Mux RX SigOk Ln#3 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten12",
     12,
     12,
     0,
     10029,
     "MAC Ch#0 TX Idle Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten13",
     13,
     13,
     0,
     10030,
     "MAC Ch#1 TX Idle Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten14",
     14,
     14,
     0,
     10031,
     "MAC Ch#2 TX Idle Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten15",
     15,
     15,
     0,
     10032,
     "MAC Ch#3 TX Idle Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable0_flds = {
    16, interrupts0_clrintenable0_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable1_fld_list[] = {
    {"setinten8",
     8,
     8,
     0,
     10033,
     "TX Path Parity Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten9", 9, 9, 0, 10034, "42"},

};
reg_decoder_t interrupts0_setintenable1_flds = {
    2, interrupts0_setintenable1_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable1_fld_list[] = {
    {"clrinten8",
     8,
     8,
     0,
     10035,
     "TX Path Parity Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10036,
     "RX Path Parity Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable1_flds = {
    2, interrupts0_clrintenable1_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable2_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10037,
     "Fifoctrl Ch#0 RX Fifo Error Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10038,
     "Fifoctrl Ch#0 TX Fifo Overflow Error Interrupt Set. When a 1 is writen "
     "to the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10039,
     "Fifoctrl Ch#0 TX Fifo DCNT Protocol Violation Error Interrupt Set. When "
     "a 1 is writen to the this bit the corresponding interrupt is enabled; "
     "when a zero is written to this bit there is no change in the interrupt "
     "enable state. Reading either the 'set' or 'clr' address for this "
     "register returns the current state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10040,
     "Fifoctrl Ch#0 TX Fifo SOF/EOF Protocol Violation Error Interrupt Set. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "enabled; when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10041,
     "Fifoctrl Ch#1 RX Fifo Error Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10042,
     "Fifoctrl Ch#1 TX Fifo Overflow Error Interrupt Set. When a 1 is writen "
     "to the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10043,
     "Fifoctrl Ch#1 TX Fifo DCNT Protocol Violation Error Interrupt Set. When "
     "a 1 is writen to the this bit the corresponding interrupt is enabled; "
     "when a zero is written to this bit there is no change in the interrupt "
     "enable state. Reading either the 'set' or 'clr' address for this "
     "register returns the current state on the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10044,
     "Fifoctrl Ch#1 TX Fifo SOF/EOF Protocol Violation Error Interrupt Set. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "enabled; when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10045,
     "Fifoctrl Ch#2 RX Fifo Error Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10046,
     "Fifoctrl Ch#2 TX Fifo Overflow Error Interrupt Set. When a 1 is writen "
     "to the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten10",
     10,
     10,
     0,
     10047,
     "Fifoctrl Ch#2 TX Fifo DCNT Protocol Violation Error Interrupt Set. When "
     "a 1 is writen to the this bit the corresponding interrupt is enabled; "
     "when a zero is written to this bit there is no change in the interrupt "
     "enable state. Reading either the 'set' or 'clr' address for this "
     "register returns the current state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10048,
     "Fifoctrl Ch#2 TX Fifo SOF/EOF Protocol Violation Error Interrupt Set. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "enabled; when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"setinten12",
     12,
     12,
     0,
     10049,
     "Fifoctrl Ch#3 RX Fifo Error Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten13",
     13,
     13,
     0,
     10050,
     "Fifoctrl Ch#3 TX Fifo Overflow Error Interrupt Set. When a 1 is writen "
     "to the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten14",
     14,
     14,
     0,
     10051,
     "Fifoctrl Ch#3 TX Fifo DCNT Protocol Violation Error Interrupt Set. When "
     "a 1 is writen to the this bit the corresponding interrupt is enabled; "
     "when a zero is written to this bit there is no change in the interrupt "
     "enable state. Reading either the 'set' or 'clr' address for this "
     "register returns the current state on the interrupt enable."},

    {"setinten15",
     15,
     15,
     0,
     10052,
     "Fifoctrl Ch#3 TX Fifo SOF/EOF Protocol Violation Error Interrupt Set. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "enabled; when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable2_flds = {
    16, interrupts0_setintenable2_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable2_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10053,
     "Fifoctrl Ch#0 RX Fifo Error Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10054,
     "Fifoctrl Ch#0 TX Fifo Overflow Error Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10055,
     "Fifoctrl Ch#0 TX Fifo DCNT Protocol Violation Error Interrupt clear. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "disabled when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10056,
     "Fifoctrl Ch#0 TX Fifo SOF/EOF Protocol Violation Error Interrupt clear. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "disabled when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10057,
     "Fifoctrl Ch#1 RX Fifo Error Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10058,
     "Fifoctrl Ch#1 TX Fifo Overflow Error Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10059,
     "Fifoctrl Ch#1 TX Fifo DCNT Protocol Violation Error Interrupt clear. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "disabled when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10060,
     "Fifoctrl Ch#1 TX Fifo SOF/EOF Protocol Violation Error Interrupt clear. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "disabled when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10061,
     "Fifoctrl Ch#2 RX Fifo Error Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10062,
     "Fifoctrl Ch#2 TX Fifo Overflow Error Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten10",
     10,
     10,
     0,
     10063,
     "Fifoctrl Ch#2 TX Fifo DCNT Protocol Violation Error Interrupt clear. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "disabled when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10064,
     "Fifoctrl Ch#2 TX Fifo SOF/EOF Protocol Violation Error Interrupt clear. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "disabled when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"clrinten12",
     12,
     12,
     0,
     10065,
     "Fifoctrl Ch#3 RX Fifo Error Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten13",
     13,
     13,
     0,
     10066,
     "Fifoctrl Ch#3 TX Fifo Overflow Error Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten14",
     14,
     14,
     0,
     10067,
     "Fifoctrl Ch#3 TX Fifo DCNT Protocol Violation Error Interrupt clear. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "disabled when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

    {"clrinten15",
     15,
     15,
     0,
     10068,
     "Fifoctrl Ch#3 TX Fifo SOF/EOF Protocol Violation Error Interrupt clear. "
     "When a 1 is writen to the this bit the corresponding interrupt is "
     "disabled when a zero is written to this bit there is no change in the "
     "interrupt enable state. Reading either the 'set' or 'clr' address for "
     "this register returns the current state on the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable2_flds = {
    16, interrupts0_clrintenable2_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable3_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10069,
     "MAC Ch#0 TX underrun Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10070,
     "MAC Ch#0 TX jabber Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10071,
     "MAC Ch#0 TX timestamp fifo overflow error Interrupt set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10072,
     "MAC Ch#0 TX timestamp fifo available Interrupt set. When a 1 is writen "
     "to the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10073,
     "MAC Ch#0 RxLocalFault Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10074,
     "MAC Ch#0 RxRemoteFault Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10075,
     "MAC Ch#0 RxCRCError Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable3_flds = {
    7, interrupts0_setintenable3_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable3_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10076,
     "MAC Ch#0 TX underrun Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10077,
     "MAC Ch#0 TX jabber Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10078,
     "MAC Ch#0 TX timestamp fifo overflow error Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10079,
     "MAC Ch#0 TX timestamp fifo available Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10080,
     "MAC Ch#0 RxLocalFault Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10081,
     "MAC Ch#0 RxRemoteFault Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10082,
     "MAC Ch#0 RxCRCError Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable3_flds = {
    7, interrupts0_clrintenable3_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable4_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10083,
     "MAC Ch#1 TX underrun Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10084,
     "MAC Ch#1 TX jabber Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10085,
     "MAC Ch#1 TX timestamp fifo overflow error Interrupt set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10086,
     "MAC Ch#1 TX timestamp fifo available Interrupt set. When a 1 is writen "
     "to the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10087,
     "MAC Ch#1 RxLocalFault Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10088,
     "MAC Ch#1 RxRemoteFault Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10089,
     "MAC Ch#1 RxCRCError Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable4_flds = {
    7, interrupts0_setintenable4_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable4_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10090,
     "MAC Ch#1 TX underrun Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10091,
     "MAC Ch#1 TX jabber Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10092,
     "MAC Ch#1 TX timestamp fifo overflow error Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10093,
     "MAC Ch#1 TX timestamp fifo available Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10094,
     "MAC Ch#1 RxLocalFault Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10095,
     "MAC Ch#1 RxRemoteFault Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10096,
     "MAC Ch#1 RxCRCError Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable4_flds = {
    7, interrupts0_clrintenable4_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable5_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10097,
     "MAC Ch#2 TX underrun Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10098,
     "MAC Ch#2 TX jabber Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10099,
     "MAC Ch#2 TX timestamp fifo overflow error Interrupt set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10100,
     "MAC Ch#2 TX timestamp fifo available Interrupt set. When a 1 is writen "
     "to the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10101,
     "MAC Ch#2 RxLocalFault Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10102,
     "MAC Ch#2 RxRemoteFault Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10103,
     "MAC Ch#2 RxCRCError Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable5_flds = {
    7, interrupts0_setintenable5_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable5_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10104,
     "MAC Ch#2 TX underrun Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10105,
     "MAC Ch#2 TX jabber Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10106,
     "MAC Ch#2 TX timestamp fifo overflow error Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10107,
     "MAC Ch#2 TX timestamp fifo available Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10108,
     "MAC Ch#2 RxLocalFault Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10109,
     "MAC Ch#2 RxRemoteFault Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10110,
     "MAC Ch#2 RxCRCError Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable5_flds = {
    7, interrupts0_clrintenable5_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable6_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10111,
     "MAC Ch#3 TX underrun Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10112,
     "MAC Ch#3 TX jabber Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10113,
     "MAC Ch#3 TX timestamp fifo overflow error Interrupt set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10114,
     "MAC Ch#3 TX timestamp fifo available Interrupt set. When a 1 is writen "
     "to the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10115,
     "MAC Ch#3 RxLocalFault Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10116,
     "MAC Ch#3 RxRemoteFault Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10117,
     "MAC Ch#3 RxCRCError Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable6_flds = {
    7, interrupts0_setintenable6_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable6_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10118,
     "MAC Ch#3 TX underrun Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10119,
     "MAC Ch#3 TX jabber Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10120,
     "MAC Ch#3 TX timestamp fifo overflow error Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10121,
     "MAC Ch#3 TX timestamp fifo available Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10122,
     "MAC Ch#3 RxLocalFault Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10123,
     "MAC Ch#3 RxRemoteFault Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10124,
     "MAC Ch#3 RxCRCError Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable6_flds = {
    7, interrupts0_clrintenable6_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable7_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10125,
     "hsmcpcs Ch#0 block lock Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10126,
     "hsmcpcs Ch#0 fault Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10127,
     "hsmcpcs Ln#0 TX gearbox fifo err Interrupt set. When a 1 is writen to "
     "the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10128,
     "hsmcpcs Ch#0 decoder trap Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10129,
     "hsmcpcs Ch#0 debug deskew overflow Interrupt set. When a 1 is writen to "
     "the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10130,
     "FC FEC #0 block lock gained Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10131,
     "FC FEC #0 Block lock lost Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10132,
     "FC FEC #0 Uncorrectable Codeword received Interrupt Set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10133,
     "FC FEC #0 Bad Codeword Received Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten10",
     10,
     10,
     0,
     10134,
     "Too many FC FECs requested Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10135,
     "hsmcpcs Ch#0 Loss of Sync Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten12",
     12,
     12,
     0,
     10136,
     "hsmcpcs Ch#0 Loss of Block Lock Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten13",
     13,
     13,
     0,
     10137,
     "hsmcpcs Ch#0 High BER event Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten14",
     14,
     14,
     0,
     10138,
     "hsmcpcs Ch#0 Error Block Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable7_flds = {
    14, interrupts0_setintenable7_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable7_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10139,
     "hsmcpcs Ch#0 block lock Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10140,
     "hsmcpcs Ch#0 fault Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10141,
     "hsmcpcs Ln#0 TX gearbox fifo err Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10142,
     "hsmcpcs Ch#0 decoder trap Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10143,
     "hsmcpcs Ch#0 debug deskew overflow Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10144,
     "FC FEC #0 block lock gained Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10145,
     "FC FEC #0 Block lock lost Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10146,
     "FC FEC #0 Uncorrectable Codeword received Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10147,
     "FC FEC #0 Bad Codeword Received Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten10",
     10,
     10,
     0,
     10148,
     "Too many FC FECs requested Interrupt Clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10149,
     "hsmcpcs Ch#0 Loss of Sync Interrupt Clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten12",
     12,
     12,
     0,
     10150,
     "hsmcpcs Ch#0 Loss of Block Lock Interrupt Clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten13",
     13,
     13,
     0,
     10151,
     "hsmcpcs Ch#0 High BER event Interrupt Clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten14",
     14,
     14,
     0,
     10152,
     "hsmcpcs Ch#0 Error Block Interrupt Clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable7_flds = {
    14, interrupts0_clrintenable7_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable8_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10153,
     "hsmcpcs Ch#1 block lock Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10154,
     "hsmcpcs Ch#1 fault Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10155,
     "hsmcpcs Ln#1 TX gearbox fifo err Interrupt set. When a 1 is writen to "
     "the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10156,
     "hsmcpcs Ch#1 decoder trap Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10157,
     "hsmcpcs Ch#1 debug deskew overflow Interrupt set. When a 1 is writen to "
     "the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10158,
     "FC FEC #1 block lock gained Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10159,
     "FC FEC #1 Block lock lost Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10160,
     "FC FEC #1 Uncorrectable Codeword received Interrupt Set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10161,
     "FC FEC #1 Bad Codeword Received Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10162,
     "hsmcpcs Ch#1 Loss of Sync Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten12",
     12,
     12,
     0,
     10163,
     "hsmcpcs Ch#1 Loss of Block Lock Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten13",
     13,
     13,
     0,
     10164,
     "hsmcpcs Ch#1 High BER event Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten14",
     14,
     14,
     0,
     10165,
     "hsmcpcs Ch#1 Error Block Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable8_flds = {
    13, interrupts0_setintenable8_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable8_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10166,
     "hsmcpcs Ch#1 block lock Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10167,
     "hsmcpcs Ch#1 fault Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10168,
     "hsmcpcs Ln#1 TX gearbox fifo err Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10169,
     "hsmcpcs Ch#1 decoder trap Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10170,
     "hsmcpcs Ch#1 debug deskew overflow Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10171,
     "FC FEC #1 block lock gained Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10172,
     "FC FEC #1 Block lock lost Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10173,
     "FC FEC #1 Uncorrectable Codeword received Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10174,
     "FC FEC #1 Bad Codeword Received Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10175,
     "hsmcpcs Ch#1 Loss of Sync Interrupt Clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten12",
     12,
     12,
     0,
     10176,
     "hsmcpcs Ch#1 Loss of Block Lock Interrupt Clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten13",
     13,
     13,
     0,
     10177,
     "hsmcpcs Ch#1 High BER event Interrupt Clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten14",
     14,
     14,
     0,
     10178,
     "hsmcpcs Ch#1 Error Block Interrupt Clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable8_flds = {
    13, interrupts0_clrintenable8_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable9_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10179,
     "hsmcpcs Ch#2 block lock Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10180,
     "hsmcpcs Ch#2 fault Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10181,
     "hsmcpcs Ln#2 TX gearbox fifo err Interrupt set. When a 1 is writen to "
     "the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10182,
     "hsmcpcs Ch#2 decoder trap Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10183,
     "hsmcpcs Ch#2 debug deskew overflow Interrupt set. When a 1 is writen to "
     "the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10184,
     "FC FEC #2 block lock gained Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10185,
     "FC FEC #2 Block lock lost Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10186,
     "FC FEC #2 Uncorrectable Codeword received Interrupt Set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10187,
     "FC FEC #2 Bad Codeword Received Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10188,
     "hsmcpcs Ch#2 Loss of Sync Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten12",
     12,
     12,
     0,
     10189,
     "hsmcpcs Ch#2 Loss of Block Lock Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten13",
     13,
     13,
     0,
     10190,
     "hsmcpcs Ch#2 High BER event Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten14",
     14,
     14,
     0,
     10191,
     "hsmcpcs Ch#2 Error Block Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable9_flds = {
    13, interrupts0_setintenable9_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable9_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10192,
     "hsmcpcs Ch#2 block lock Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10193,
     "hsmcpcs Ch#2 fault Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10194,
     "hsmcpcs Ln#2 TX gearbox fifo err Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10195,
     "hsmcpcs Ch#2 decoder trap Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10196,
     "hsmcpcs Ch#2 debug deskew overflow Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10197,
     "FC FEC #2 block lock gained Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10198,
     "FC FEC #2 Block lock lost Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10199,
     "FC FEC #2 Uncorrectable Codeword received Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10200,
     "FC FEC #2 Bad Codeword Received Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10201,
     "hsmcpcs Ch#2 Loss of Sync Interrupt Clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten12",
     12,
     12,
     0,
     10202,
     "hsmcpcs Ch#2 Loss of Block Lock Interrupt Clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten13",
     13,
     13,
     0,
     10203,
     "hsmcpcs Ch#2 High BER event Interrupt Clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten14",
     14,
     14,
     0,
     10204,
     "hsmcpcs Ch#2 Error Block Interrupt Clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable9_flds = {
    13, interrupts0_clrintenable9_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable10_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10205,
     "hsmcpcs Ch#3 block lock Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10206,
     "hsmcpcs Ch#3 fault Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10207,
     "hsmcpcs Ln#3 TX gearbox fifo err Interrupt set. When a 1 is writen to "
     "the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10208,
     "hsmcpcs Ch#3 decoder trap Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10209,
     "hsmcpcs Ch#3 debug deskew overflow Interrupt set. When a 1 is writen to "
     "the this bit the corresponding interrupt is enabled; when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10210,
     "FC FEC #3 block lock gained Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10211,
     "FC FEC #3 Block lock lost Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10212,
     "FC FEC #3 Uncorrectable Codeword received Interrupt Set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10213,
     "FC FEC #3 Bad Codeword Received Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10214,
     "hsmcpcs Ch#3 Loss of Sync Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten12",
     12,
     12,
     0,
     10215,
     "hsmcpcs Ch#3 Loss of Block Lock Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten13",
     13,
     13,
     0,
     10216,
     "hsmcpcs Ch#3 High BER event Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten14",
     14,
     14,
     0,
     10217,
     "hsmcpcs Ch#3 Error Block Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable10_flds = {
    13, interrupts0_setintenable10_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable10_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10218,
     "hsmcpcs Ch#3 block lock Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10219,
     "hsmcpcs Ch#3 fault Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10220,
     "hsmcpcs Ln#3 TX gearbox fifo err Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10221,
     "hsmcpcs Ch#3 decoder trap Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10222,
     "hsmcpcs Ch#3 debug deskew overflow Interrupt clear. When a 1 is writen "
     "to the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10223,
     "FC FEC #3 block lock gained Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10224,
     "FC FEC #3 Block lock lost Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10225,
     "FC FEC #3 Uncorrectable Codeword received Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10226,
     "FC FEC #3 Bad Codeword Received Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10227,
     "hsmcpcs Ch#3 Loss of Sync Interrupt Clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten12",
     12,
     12,
     0,
     10228,
     "hsmcpcs Ch#3 Loss of Block Lock Interrupt Clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

    {"clrinten13",
     13,
     13,
     0,
     10229,
     "hsmcpcs Ch#3 High BER event Interrupt Clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten14",
     14,
     14,
     0,
     10230,
     "hsmcpcs Ch#3 Error Block Interrupt Clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable10_flds = {
    13, interrupts0_clrintenable10_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable11_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10231,
     "lsmcpcs Ch #0 AN Done Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10232,
     "RSFEC #0 AM lost lane 0 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10233,
     "RSFEC #0 AM lost lane 1 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10234,
     "RSFEC #0 AM lost lane 2 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10235,
     "RSFEC #0 AM lost lane 3 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10236,
     "RSFEC #0 uncorrectable Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10237,
     "RSFEC #0 deskew lost Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10238,
     "RSFEC #0 BER over threshold Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10239,
     "FC FEC #4 block lock gained Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10240,
     "FC FEC #4 Block lock lost Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten10",
     10,
     10,
     0,
     10241,
     "FC FEC #4 Uncorrectable Codeword received Interrupt Set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10242,
     "FC FEC #4 Bad Codeword Received Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable11_flds = {
    12, interrupts0_setintenable11_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable11_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10243,
     "lsmcpcs Ch #0 AN Done Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10244,
     "RSFEC #0 AM lost lane 0 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10245,
     "RSFEC #0 AM lost lane 1 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10246,
     "RSFEC #0 AM lost lane 2 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10247,
     "RSFEC #0 AM lost lane 3 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10248,
     "RSFEC #0 uncorrectable Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10249,
     "RSFEC #0 deskew lost Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10250,
     "RSFEC #0 BER over threshold Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10251,
     "FC FEC #4 block lock gained Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10252,
     "FC FEC #4 Block lock lost Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten10",
     10,
     10,
     0,
     10253,
     "FC FEC #4 Uncorrectable Codeword received Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10254,
     "FC FEC #4 Bad Codeword Received Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable11_flds = {
    12, interrupts0_clrintenable11_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable12_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10255,
     "lsmcpcs Ch #1 AN Done Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10256,
     "RSFEC #1 AM lost lane 0 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10257,
     "RSFEC #1 AM lost lane 1 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10258,
     "RSFEC #1 AM lost lane 2 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10259,
     "RSFEC #1 AM lost lane 3 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10260,
     "RSFEC #1 uncorrectable Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10261,
     "RSFEC #1 deskew lost Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10262,
     "RSFEC #1 BER over threshold Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10263,
     "FC FEC #5 block lock gained Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10264,
     "FC FEC #5 Block lock lost Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten10",
     10,
     10,
     0,
     10265,
     "FC FEC #5 Uncorrectable Codeword received Interrupt Set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10266,
     "FC FEC #5 Bad Codeword Received Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable12_flds = {
    12, interrupts0_setintenable12_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable12_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10267,
     "lsmcpcs Ch #1 AN Done Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10268,
     "RSFEC #1 AM lost lane 0 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10269,
     "RSFEC #1 AM lost lane 1 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10270,
     "RSFEC #1 AM lost lane 2 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10271,
     "RSFEC #1 AM lost lane 3 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10272,
     "RSFEC #1 uncorrectable Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10273,
     "RSFEC #1 deskew lost Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10274,
     "RSFEC #1 BER over threshold Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10275,
     "FC FEC #5 block lock gained Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10276,
     "FC FEC #5 Block lock lost Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten10",
     10,
     10,
     0,
     10277,
     "FC FEC #5 Uncorrectable Codeword received Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10278,
     "FC FEC #5 Bad Codeword Received Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable12_flds = {
    12, interrupts0_clrintenable12_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable13_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10279,
     "lsmcpcs Ch #2 AN Done Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10280,
     "RSFEC #2 AM lost lane 0 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10281,
     "RSFEC #2 AM lost lane 1 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10282,
     "RSFEC #2 AM lost lane 2 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10283,
     "RSFEC #2 AM lost lane 3 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10284,
     "RSFEC #2 uncorrectable Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10285,
     "RSFEC #2 deskew lost Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10286,
     "RSFEC #2 BER over threshold Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10287,
     "FC FEC #6 block lock gained Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10288,
     "FC FEC #6 Block lock lost Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten10",
     10,
     10,
     0,
     10289,
     "FC FEC #6 Uncorrectable Codeword received Interrupt Set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10290,
     "FC FEC #6 Bad Codeword Received Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable13_flds = {
    12, interrupts0_setintenable13_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable13_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10291,
     "lsmcpcs Ch #2 AN Done Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10292,
     "RSFEC #2 AM lost lane 0 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10293,
     "RSFEC #2 AM lost lane 1 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10294,
     "RSFEC #2 AM lost lane 2 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10295,
     "RSFEC #2 AM lost lane 3 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10296,
     "RSFEC #2 uncorrectable Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10297,
     "RSFEC #2 deskew lost Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10298,
     "RSFEC #2 BER over threshold Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10299,
     "FC FEC #6 block lock gained Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10300,
     "FC FEC #6 Block lock lost Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten10",
     10,
     10,
     0,
     10301,
     "FC FEC #6 Uncorrectable Codeword received Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10302,
     "FC FEC #6 Bad Codeword Received Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable13_flds = {
    12, interrupts0_clrintenable13_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable14_fld_list[] = {
    {"setinten0",
     0,
     0,
     0,
     10303,
     "lsmcpcs Ch #3 AN Done Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten1",
     1,
     1,
     0,
     10304,
     "RSFEC #3 AM lost lane 0 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten2",
     2,
     2,
     0,
     10305,
     "RSFEC #3 AM lost lane 1 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten3",
     3,
     3,
     0,
     10306,
     "RSFEC #3 AM lost lane 2 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten4",
     4,
     4,
     0,
     10307,
     "RSFEC #3 AM lost lane 3 Interrupt set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten5",
     5,
     5,
     0,
     10308,
     "RSFEC #3 uncorrectable Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten6",
     6,
     6,
     0,
     10309,
     "RSFEC #3 deskew lost Interrupt set. When a 1 is writen to the this bit "
     "the corresponding interrupt is enabled; when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten7",
     7,
     7,
     0,
     10310,
     "RSFEC #3 BER over threshold Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10311,
     "FC FEC #7 block lock gained Interrupt set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10312,
     "FC FEC #7 Block lock lost Interrupt Set. When a 1 is writen to the this "
     "bit the corresponding interrupt is enabled; when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"setinten10",
     10,
     10,
     0,
     10313,
     "FC FEC #7 Uncorrectable Codeword received Interrupt Set. When a 1 is "
     "writen to the this bit the corresponding interrupt is enabled; when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"setinten11",
     11,
     11,
     0,
     10314,
     "FC FEC #7 Bad Codeword Received Interrupt Set. When a 1 is writen to the "
     "this bit the corresponding interrupt is enabled; when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable14_flds = {
    12, interrupts0_setintenable14_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable14_fld_list[] = {
    {"clrinten0",
     0,
     0,
     0,
     10315,
     "lsmcpcs Ch #3 AN Done Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten1",
     1,
     1,
     0,
     10316,
     "RSFEC #3 AM lost lane 0 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten2",
     2,
     2,
     0,
     10317,
     "RSFEC #3 AM lost lane 1 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten3",
     3,
     3,
     0,
     10318,
     "RSFEC #3 AM lost lane 2 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten4",
     4,
     4,
     0,
     10319,
     "RSFEC #3 AM lost lane 3 Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten5",
     5,
     5,
     0,
     10320,
     "RSFEC #3 uncorrectable Interrupt clear. When a 1 is writen to the this "
     "bit the corresponding interrupt is disabled when a zero is written to "
     "this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten6",
     6,
     6,
     0,
     10321,
     "RSFEC #3 deskew lost Interrupt clear. When a 1 is writen to the this bit "
     "the corresponding interrupt is disabled when a zero is written to this "
     "bit there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten7",
     7,
     7,
     0,
     10322,
     "RSFEC #3 BER over threshold Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10323,
     "FC FEC #7 block lock gained Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10324,
     "FC FEC #7 Block lock lost Interrupt clear. When a 1 is writen to the "
     "this bit the corresponding interrupt is disabled when a zero is written "
     "to this bit there is no change in the interrupt enable state. Reading "
     "either the 'set' or 'clr' address for this register returns the current "
     "state on the interrupt enable."},

    {"clrinten10",
     10,
     10,
     0,
     10325,
     "FC FEC #7 Uncorrectable Codeword received Interrupt clear. When a 1 is "
     "writen to the this bit the corresponding interrupt is disabled when a "
     "zero is written to this bit there is no change in the interrupt enable "
     "state. Reading either the 'set' or 'clr' address for this register "
     "returns the current state on the interrupt enable."},

    {"clrinten11",
     11,
     11,
     0,
     10326,
     "FC FEC #7 Bad Codeword Received Interrupt clear. When a 1 is writen to "
     "the this bit the corresponding interrupt is disabled when a zero is "
     "written to this bit there is no change in the interrupt enable state. "
     "Reading either the 'set' or 'clr' address for this register returns the "
     "current state on the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable14_flds = {
    12, interrupts0_clrintenable14_fld_list, 16};

reg_decoder_fld_t interrupts0_setintenable15_fld_list[] = {
    {"setinten7",
     7,
     7,
     0,
     10327,
     "BPAN Ch#0 Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten8",
     8,
     8,
     0,
     10328,
     "BPAN Ch#1 Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten9",
     9,
     9,
     0,
     10329,
     "BPAN Ch#2 Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"setinten10",
     10,
     10,
     0,
     10330,
     "BPAN Ch#3 Interrupt set. When a 1 is writen to the this bit the "
     "corresponding interrupt is enabled; when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_setintenable15_flds = {
    4, interrupts0_setintenable15_fld_list, 16};

reg_decoder_fld_t interrupts0_clrintenable15_fld_list[] = {
    {"clrinten7",
     7,
     7,
     0,
     10331,
     "BPAN Ch#0 Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten8",
     8,
     8,
     0,
     10332,
     "BPAN Ch#1 Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten9",
     9,
     9,
     0,
     10333,
     "BPAN Ch#2 Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

    {"clrinten10",
     10,
     10,
     0,
     10334,
     "BPAN Ch#3 Interrupt clear. When a 1 is writen to the this bit the "
     "corresponding interrupt is disabled when a zero is written to this bit "
     "there is no change in the interrupt enable state. Reading either the "
     "'set' or 'clr' address for this register returns the current state on "
     "the interrupt enable."},

};
reg_decoder_t interrupts0_clrintenable15_flds = {
    4, interrupts0_clrintenable15_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat0_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10335,
     "Fifoctrl Ch#0 RX sync Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10336,
     "Fifoctrl Ch#1 RX sync Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10337,
     "Fifoctrl Ch#2 RX sync Interrupt status. Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10338,
     "Fifoctrl Ch#3 RX sync Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10339,
     "Mac Ch#0 RX fault Interrupt status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10340,
     "Mac Ch#1 RX fault Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10341,
     "Mac Ch#2 RX fault Interrupt status. Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10342,
     "Mac Ch#3 RX fault Interrupt status. Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10343,
     "SerDes Mux RX SigOk Ln#0 Interrupt status. Interrupt Status"},

    {"stat9",
     9,
     9,
     0,
     10344,
     "SerDes Mux RX SigOk Ln#1 Interrupt status. Interrupt Status"},

    {"stat10",
     10,
     10,
     0,
     10345,
     "SerDes Mux RX SigOk Ln#2 Interrupt status. Interrupt Status"},

    {"stat11",
     11,
     11,
     0,
     10346,
     "SerDes Mux RX SigOk Ln#3 Interrupt status. Interrupt Status"},

    {"stat12",
     12,
     12,
     0,
     10347,
     "MAC Ch#0 TX Idle Interrupt status. Interrupt Status"},

    {"stat13",
     13,
     13,
     0,
     10348,
     "MAC Ch#1 TX Idle Interrupt status. Interrupt Status"},

    {"stat14",
     14,
     14,
     0,
     10349,
     "MAC Ch#2 TX Idle Interrupt status. Interrupt Status"},

    {"stat15",
     15,
     15,
     0,
     10350,
     "MAC Ch#3 TX Idle Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat0_flds = {
    16, interrupts0_intstat0_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr0_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10351,
     "Fifoctrl Ch#0 RX sync Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10352,
     "Fifoctrl Ch#1 RX sync Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10353,
     "Fifoctrl Ch#2 RX sync Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10354,
     "Fifoctrl Ch#3 RX sync Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10355,
     "Mac Ch#0 RX fault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10356,
     "Mac Ch#1 RX fault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr6",
     6,
     6,
     0,
     10357,
     "Mac Ch#2 RX fault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10358,
     "Mac Ch#3 RX fault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10359,
     "SerDes Mux RX SigOk Ln#0 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr9",
     9,
     9,
     0,
     10360,
     "SerDes Mux RX SigOk Ln#1 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr10",
     10,
     10,
     0,
     10361,
     "SerDes Mux RX SigOk Ln#2 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr11",
     11,
     11,
     0,
     10362,
     "SerDes Mux RX SigOk Ln#3 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr12",
     12,
     12,
     0,
     10363,
     "MAC Ch#0 TX Idle Interrupt status clear. Clear Interrupt Status Register "
     "(if interrupt is level must also be cleared at source)"},

    {"clr13",
     13,
     13,
     0,
     10364,
     "MAC Ch#1 TX Idle Interrupt status clear. Clear Interrupt Status Register "
     "(if interrupt is level must also be cleared at source)"},

    {"clr14",
     14,
     14,
     0,
     10365,
     "MAC Ch#2 TX Idle Interrupt status clear. Clear Interrupt Status Register "
     "(if interrupt is level must also be cleared at source)"},

    {"clr15",
     15,
     15,
     0,
     10366,
     "MAC Ch#3 TX Idle Interrupt status clear. Clear Interrupt Status Register "
     "(if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr0_flds = {16, interrupts0_intclr0_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat1_fld_list[] = {
    {"stat8",
     8,
     8,
     0,
     10367,
     "RX Path Parity Interrupt status. Interrupt Status"},

    {"stat9",
     9,
     9,
     0,
     10368,
     "TX Path Parity Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat1_flds = {
    2, interrupts0_intstat1_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr1_fld_list[] = {
    {"clr8",
     8,
     8,
     0,
     10369,
     "RX Path Parity Interrupt status clear. Clear Interrupt Status Register "
     "(if interrupt is level must also be cleared at source)"},

    {"clr9",
     9,
     9,
     0,
     10370,
     "TX Path Parity Interrupt status clear. Clear Interrupt Status Register "
     "(if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr1_flds = {2, interrupts0_intclr1_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat2_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10371,
     "Fifoctrl Ch#0 RX Fifo Error Interrupt Status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10372,
     "Fifoctrl Ch#0 TX Fifo Overflow Error Interrupt Status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10373,
     "Fifoctrl Ch#0 TX Fifo DCNT Protocol Violation Error Interrupt Status. "
     "Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10374,
     "Fifoctrl Ch#0 TX Fifo SOF/EOF Protocol Violation Error Interrupt Status. "
     "Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10375,
     "Fifoctrl Ch#1 RX Fifo Error Interrupt Status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10376,
     "Fifoctrl Ch#1 TX Fifo Overflow Error Interrupt Status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10377,
     "Fifoctrl Ch#1 TX Fifo DCNT Protocol Violation Error Interrupt Status. "
     "Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10378,
     "Fifoctrl Ch#1 TX Fifo SOF/EOF Protocol Violation Error Interrupt Status. "
     "Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10379,
     "Fifoctrl Ch#2 RX Fifo Error Interrupt Status. Interrupt Status"},

    {"stat9",
     9,
     9,
     0,
     10380,
     "Fifoctrl Ch#2 TX Fifo Overflow Error Interrupt Status. Interrupt Status"},

    {"stat10",
     10,
     10,
     0,
     10381,
     "Fifoctrl Ch#2 TX Fifo DCNT Protocol Violation Error Interrupt Status. "
     "Interrupt Status"},

    {"stat11",
     11,
     11,
     0,
     10382,
     "Fifoctrl Ch#2 TX Fifo SOF/EOF Protocol Violation Error Interrupt Status. "
     "Interrupt Status"},

    {"stat12",
     12,
     12,
     0,
     10383,
     "Fifoctrl Ch#3 RX Fifo Error Interrupt Status. Interrupt Status"},

    {"stat13",
     13,
     13,
     0,
     10384,
     "Fifoctrl Ch#3 TX Fifo Overflow Error Interrupt Status. Interrupt Status"},

    {"stat14",
     14,
     14,
     0,
     10385,
     "Fifoctrl Ch#3 TX Fifo DCNT Protocol Violation Error Interrupt Status. "
     "Interrupt Status"},

    {"stat15",
     15,
     15,
     0,
     10386,
     "Fifoctrl Ch#3 TX Fifo SOF/EOF Protocol Violation Error Interrupt Status. "
     "Interrupt Status"},

};
reg_decoder_t interrupts0_intstat2_flds = {
    16, interrupts0_intstat2_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr2_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10387,
     "Fifoctrl Ch#0 RX Fifo Error Interrupt Status Clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10388,
     "Fifoctrl Ch#0 TX Fifo Overflow Error Interrupt Status Clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr2",
     2,
     2,
     0,
     10389,
     "Fifoctrl Ch#0 TX Fifo DCNT Protocol Violation Error Interrupt Status "
     "Clear. Clear Interrupt Status Register (if interrupt is level must also "
     "be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10390,
     "Fifoctrl Ch#0 TX Fifo SOF/EOF Protocol Violation Error Interrupt Status "
     "Clear. Clear Interrupt Status Register (if interrupt is level must also "
     "be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10391,
     "Fifoctrl Ch#1 RX Fifo Error Interrupt Status Clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10392,
     "Fifoctrl Ch#1 TX Fifo Overflow Error Interrupt Status Clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr6",
     6,
     6,
     0,
     10393,
     "Fifoctrl Ch#1 TX Fifo DCNT Protocol Violation Error Interrupt Status "
     "Clear. Clear Interrupt Status Register (if interrupt is level must also "
     "be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10394,
     "Fifoctrl Ch#1 TX Fifo SOF/EOF Protocol Violation Error Interrupt Status "
     "Clear. Clear Interrupt Status Register (if interrupt is level must also "
     "be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10395,
     "Fifoctrl Ch#2 RX Fifo Error Interrupt Status Clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr9",
     9,
     9,
     0,
     10396,
     "Fifoctrl Ch#2 TX Fifo Overflow Error Interrupt Status Clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr10",
     10,
     10,
     0,
     10397,
     "Fifoctrl Ch#2 TX Fifo DCNT Protocol Violation Error Interrupt Status "
     "Clear. Clear Interrupt Status Register (if interrupt is level must also "
     "be cleared at source)"},

    {"clr11",
     11,
     11,
     0,
     10398,
     "Fifoctrl Ch#2 TX Fifo SOF/EOF Protocol Violation Error Interrupt Status "
     "Clear. Clear Interrupt Status Register (if interrupt is level must also "
     "be cleared at source)"},

    {"clr12",
     12,
     12,
     0,
     10399,
     "Fifoctrl Ch#3 RX Fifo Error Interrupt Status Clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr13",
     13,
     13,
     0,
     10400,
     "Fifoctrl Ch#3 TX Fifo Overflow Error Interrupt Status Clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr14",
     14,
     14,
     0,
     10401,
     "Fifoctrl Ch#3 TX Fifo DCNT Protocol Violation Error Interrupt Status "
     "Clear. Clear Interrupt Status Register (if interrupt is level must also "
     "be cleared at source)"},

    {"clr15",
     15,
     15,
     0,
     10402,
     "Fifoctrl Ch#3 TX Fifo SOF/EOF Protocol Violation Error Interrupt Status "
     "Clear. Clear Interrupt Status Register (if interrupt is level must also "
     "be cleared at source)"},

};
reg_decoder_t interrupts0_intclr2_flds = {16, interrupts0_intclr2_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat3_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10403,
     "MAC Ch#0 TX underrun Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10404,
     "MAC Ch#0 TX jabber Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10405,
     "MAC Ch#0 TX timestamp fifo overflow error Interrupt status. Interrupt "
     "Status"},

    {"stat3",
     3,
     3,
     0,
     10406,
     "MAC Ch#0 TX timestamp fifo available Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10407,
     "MAC Ch#0 RxLocalFault Interrupt status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10408,
     "MAC Ch#0 RxRemoteFault Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10409,
     "MAC Ch#0 RxCRCError Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat3_flds = {
    7, interrupts0_intstat3_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr3_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10410,
     "MAC Ch#0 TX underrun Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10411,
     "MAC Ch#0 TX jabber Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10412,
     "MAC Ch#0 TX timestamp fifo overflow error Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr3",
     3,
     3,
     0,
     10413,
     "MAC Ch#0 TX timestamp fifo available Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr4",
     4,
     4,
     0,
     10414,
     "MAC Ch#0 RxLocalFault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10415,
     "MAC Ch#0 RxRemoteFault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr6",
     6,
     6,
     0,
     10416,
     "MAC Ch#0 RxCRCError Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr3_flds = {7, interrupts0_intclr3_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat4_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10417,
     "MAC Ch#1 TX underrun Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10418,
     "MAC Ch#1 TX jabber Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10419,
     "MAC Ch#1 TX timestamp fifo overflow error Interrupt status. Interrupt "
     "Status"},

    {"stat3",
     3,
     3,
     0,
     10420,
     "MAC Ch#1 TX timestamp fifo available Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10421,
     "MAC Ch#1 RxLocalFault Interrupt status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10422,
     "MAC Ch#1 RxRemoteFault Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10423,
     "MAC Ch#1 RxCRCError Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat4_flds = {
    7, interrupts0_intstat4_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr4_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10424,
     "MAC Ch#1 TX underrun Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10425,
     "MAC Ch#1 TX jabber Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10426,
     "MAC Ch#1 TX timestamp fifo overflow error Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr3",
     3,
     3,
     0,
     10427,
     "MAC Ch#1 TX timestamp fifo available Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr4",
     4,
     4,
     0,
     10428,
     "MAC Ch#1 RxLocalFault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10429,
     "MAC Ch#1 RxRemoteFault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr6",
     6,
     6,
     0,
     10430,
     "MAC Ch#1 RxCRCError Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr4_flds = {7, interrupts0_intclr4_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat5_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10431,
     "MAC Ch#2 TX underrun Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10432,
     "MAC Ch#2 TX jabber Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10433,
     "MAC Ch#0 TX timestamp fifo overflow error Interrupt status. Interrupt "
     "Status"},

    {"stat3",
     3,
     3,
     0,
     10434,
     "MAC Ch#0 TX timestamp fifo available Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10435,
     "MAC Ch#2 RxLocalFault Interrupt status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10436,
     "MAC Ch#2 RxRemoteFault Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10437,
     "MAC Ch#2 RxCRCError Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat5_flds = {
    7, interrupts0_intstat5_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr5_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10438,
     "MAC Ch#2 TX underrun Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10439,
     "MAC Ch#2 TX jabber Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10440,
     "MAC Ch#2 TX timestamp fifo overflow error Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr3",
     3,
     3,
     0,
     10441,
     "MAC Ch#2 TX timestamp fifo available Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr4",
     4,
     4,
     0,
     10442,
     "MAC Ch#2 RxLocalFault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10443,
     "MAC Ch#2 RxRemoteFault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr6",
     6,
     6,
     0,
     10444,
     "MAC Ch#2 RxCRCError Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr5_flds = {7, interrupts0_intclr5_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat6_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10445,
     "MAC Ch#3 TX underrun Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10446,
     "MAC Ch#3 TX jabber Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10447,
     "MAC Ch#3 TX timestamp fifo overflow error Interrupt status. Interrupt "
     "Status"},

    {"stat3",
     3,
     3,
     0,
     10448,
     "MAC Ch#3 TX timestamp fifo available Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10449,
     "MAC Ch#3 RxLocalFault Interrupt status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10450,
     "MAC Ch#3 RxRemoteFault Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10451,
     "MAC Ch#3 RxCRCError Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat6_flds = {
    7, interrupts0_intstat6_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr6_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10452,
     "MAC Ch#3 TX underrun Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10453,
     "MAC Ch#3 TX jabber Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10454,
     "MAC Ch#3 TX timestamp fifo overflow error Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr3",
     3,
     3,
     0,
     10455,
     "MAC Ch#3 TX timestamp fifo available Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr4",
     4,
     4,
     0,
     10456,
     "MAC Ch#3 RxLocalFault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10457,
     "MAC Ch#3 RxRemoteFault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr6",
     6,
     6,
     0,
     10458,
     "MAC Ch#3 RxCRCError Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr6_flds = {7, interrupts0_intclr6_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat7_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10459,
     "hsmcpcs Ch#0 block lock Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10460,
     "hsmcpcs Ch#0 fault Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10461,
     "hsmcpcs Ln#0 TX gearbox fifo err Interrupt status. Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10462,
     "hsmcpcs Ch#0 decoder trap Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10463,
     "hsmcpcs Ch#0 debug deskew overflow Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10464,
     "FC FEC #0 block lock gained Interrupt status. Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10465,
     "FC FEC #0 Block lock lost Interrupt status. Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10466,
     "FC FEC #0 Uncorrectable Codeword received Interrupt status. Interrupt "
     "Status"},

    {"stat9",
     9,
     9,
     0,
     10467,
     "FC FEC #0 Bad Codeword Received Interrupt status. Interrupt Status"},

    {"stat10",
     10,
     10,
     0,
     10468,
     "Too many FC FECs requested Interrupt status. Interrupt Status"},

    {"stat11",
     11,
     11,
     0,
     10469,
     "hsmcpcs Ch#0 Loss of Sync Interrupt Status. Interrupt Status"},

    {"stat12",
     12,
     12,
     0,
     10470,
     "hsmcpcs Ch#0 Loss of Block Lock Interrupt Status. Interrupt Status"},

    {"stat13",
     13,
     13,
     0,
     10471,
     "hsmcpcs Ch#0 High BER event Interrupt Status. Interrupt Status"},

    {"stat14",
     14,
     14,
     0,
     10472,
     "hsmcpcs Ch#0 Error Block Interrupt Status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat7_flds = {
    14, interrupts0_intstat7_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr7_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10473,
     "hsmcpcs Ch#0 block lock Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10474,
     "hsmcpcs Ch#0 fault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10475,
     "hsmcpcs Ln#0 TX gearbox fifo err Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10476,
     "hsmcpcs Ch#0 decoder trap Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10477,
     "hsmcpcs Ch#0 debug deskew overflow Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr6",
     6,
     6,
     0,
     10478,
     "FC FEC #0 block lock gained Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10479,
     "FC FEC #0 Block lock lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10480,
     "FC FEC #0 Uncorrectable Codeword received Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr9",
     9,
     9,
     0,
     10481,
     "FC FEC #0 Bad Codeword Received Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr10",
     10,
     10,
     0,
     10482,
     "Too many FC FECs requested status clear. Clear Interrupt Status Register "
     "(if interrupt is level must also be cleared at source)"},

    {"clr11",
     11,
     11,
     0,
     10483,
     "hsmcpcs Ch#0 Loss of Sync Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr12",
     12,
     12,
     0,
     10484,
     "hsmcpcs Ch#0 Loss of Block Lock Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr13",
     13,
     13,
     0,
     10485,
     "hsmcpcs Ch#0 High BER event Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr14",
     14,
     14,
     0,
     10486,
     "hsmcpcs Ch#0 Error Block Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr7_flds = {14, interrupts0_intclr7_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat8_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10487,
     "hsmcpcs Ch#1 block lock Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10488,
     "hsmcpcs Ch#1 fault Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10489,
     "hsmcpcs Ln#1 TX gearbox fifo err Interrupt status. Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10490,
     "hsmcpcs Ch#1 decoder trap Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10491,
     "hsmcpcs Ch#1 debug deskew overflow Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10492,
     "FC FEC #1 block lock gained Interrupt status. Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10493,
     "FC FEC #1 Block lock lost Interrupt status. Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10494,
     "FC FEC #1 Uncorrectable Codeword received Interrupt status. Interrupt "
     "Status"},

    {"stat9",
     9,
     9,
     0,
     10495,
     "FC FEC #1 Bad Codeword Received Interrupt status. Interrupt Status"},

    {"stat11",
     11,
     11,
     0,
     10496,
     "hsmcpcs Ch#1 Loss of Sync Interrupt Status. Interrupt Status"},

    {"stat12",
     12,
     12,
     0,
     10497,
     "hsmcpcs Ch#1 Loss of Block Lock Interrupt Status. Interrupt Status"},

    {"stat13",
     13,
     13,
     0,
     10498,
     "hsmcpcs Ch#1 High BER event Interrupt Status. Interrupt Status"},

    {"stat14",
     14,
     14,
     0,
     10499,
     "hsmcpcs Ch#1 Error Block Interrupt Status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat8_flds = {
    13, interrupts0_intstat8_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr8_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10500,
     "hsmcpcs Ch#1 block lock Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10501,
     "hsmcpcs Ch#1 fault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10502,
     "hsmcpcs Ln#1 TX gearbox fifo err Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10503,
     "hsmcpcs Ch#1 decoder trap Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10504,
     "hsmcpcs Ch#1 debug deskew overflow Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr6",
     6,
     6,
     0,
     10505,
     "FC FEC #1 block lock gained Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10506,
     "FC FEC #1 Block lock lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10507,
     "FC FEC #1 Uncorrectable Codeword received Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr9",
     9,
     9,
     0,
     10508,
     "FC FEC #1 Bad Codeword Received Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr11",
     11,
     11,
     0,
     10509,
     "hsmcpcs Ch#1 Loss of Sync Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr12",
     12,
     12,
     0,
     10510,
     "hsmcpcs Ch#1 Loss of Block Lock Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr13",
     13,
     13,
     0,
     10511,
     "hsmcpcs Ch#1 High BER event Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr14",
     14,
     14,
     0,
     10512,
     "hsmcpcs Ch#1 Error Block Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr8_flds = {13, interrupts0_intclr8_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat9_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10513,
     "hsmcpcs Ch#2 block lock Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10514,
     "hsmcpcs Ch#2 fault Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10515,
     "hsmcpcs Ln#2 TX gearbox fifo err Interrupt status. Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10516,
     "hsmcpcs Ch#2 decoder trap Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10517,
     "hsmcpcs Ch#2 debug deskew overflow Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10518,
     "FC FEC #2 block lock gained Interrupt status. Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10519,
     "FC FEC #2 Block lock lost Interrupt status. Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10520,
     "FC FEC #2 Uncorrectable Codeword received Interrupt status. Interrupt "
     "Status"},

    {"stat9",
     9,
     9,
     0,
     10521,
     "FC FEC #2 Bad Codeword Received Interrupt status. Interrupt Status"},

    {"stat11",
     11,
     11,
     0,
     10522,
     "hsmcpcs Ch#2 Loss of Sync Interrupt Status. Interrupt Status"},

    {"stat12",
     12,
     12,
     0,
     10523,
     "hsmcpcs Ch#2 Loss of Block Lock Interrupt Status. Interrupt Status"},

    {"stat13",
     13,
     13,
     0,
     10524,
     "hsmcpcs Ch#2 High BER event Interrupt Status. Interrupt Status"},

    {"stat14",
     14,
     14,
     0,
     10525,
     "hsmcpcs Ch#2 Error Block Interrupt Status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat9_flds = {
    13, interrupts0_intstat9_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr9_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10526,
     "hsmcpcs Ch#2 block lock Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10527,
     "hsmcpcs Ch#2 fault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10528,
     "hsmcpcs Ln#2 TX gearbox fifo err Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10529,
     "hsmcpcs Ch#2 decoder trap Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10530,
     "hsmcpcs Ch#2 debug deskew overflow Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr6",
     6,
     6,
     0,
     10531,
     "FC FEC #2 block lock gained Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10532,
     "FC FEC #2 Block lock lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10533,
     "FC FEC #2 Uncorrectable Codeword received Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr9",
     9,
     9,
     0,
     10534,
     "FC FEC #2 Bad Codeword Received Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr11",
     11,
     11,
     0,
     10535,
     "hsmcpcs Ch#2 Loss of Sync Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr12",
     12,
     12,
     0,
     10536,
     "hsmcpcs Ch#2 Loss of Block Lock Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr13",
     13,
     13,
     0,
     10537,
     "hsmcpcs Ch#2 High BER event Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr14",
     14,
     14,
     0,
     10538,
     "hsmcpcs Ch#2 Error Block Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr9_flds = {13, interrupts0_intclr9_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat10_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10539,
     "hsmcpcs Ch#3 block lock Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10540,
     "hsmcpcs Ch#3 fault Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10541,
     "hsmcpcs Ln#3 TX gearbox fifo err Interrupt status. Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10542,
     "hsmcpcs Ch#3 decoder trap Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10543,
     "hsmcpcs Ch#3 debug deskew overflow Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10544,
     "FC FEC #3 block lock gained Interrupt status. Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10545,
     "FC FEC #3 Block lock lost Interrupt status. Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10546,
     "FC FEC #3 Uncorrectable Codeword received Interrupt status. Interrupt "
     "Status"},

    {"stat9",
     9,
     9,
     0,
     10547,
     "FC FEC #3 Bad Codeword Received Interrupt status. Interrupt Status"},

    {"stat11",
     11,
     11,
     0,
     10548,
     "hsmcpcs Ch#3 Loss of Sync Interrupt Status. Interrupt Status"},

    {"stat12",
     12,
     12,
     0,
     10549,
     "hsmcpcs Ch#3 Loss of Block Lock Interrupt Status. Interrupt Status"},

    {"stat13",
     13,
     13,
     0,
     10550,
     "hsmcpcs Ch#3 High BER event Interrupt Status. Interrupt Status"},

    {"stat14",
     14,
     14,
     0,
     10551,
     "hsmcpcs Ch#3 Error Block Interrupt Status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat10_flds = {
    13, interrupts0_intstat10_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr10_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10552,
     "hsmcpcs Ch#3 block lock Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10553,
     "hsmcpcs Ch#3 fault Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10554,
     "hsmcpcs Ln#3 TX gearbox fifo err Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10555,
     "hsmcpcs Ch#3 decoder trap Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10556,
     "hsmcpcs Ch#3 debug deskew overflow Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr6",
     6,
     6,
     0,
     10557,
     "FC FEC #3 block lock gained Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10558,
     "FC FEC #3 Block lock lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10559,
     "FC FEC #3 Uncorrectable Codeword received Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr9",
     9,
     9,
     0,
     10560,
     "FC FEC #3 Bad Codeword Received Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr11",
     11,
     11,
     0,
     10561,
     "hsmcpcs Ch#3 Loss of Sync Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr12",
     12,
     12,
     0,
     10562,
     "hsmcpcs Ch#3 Loss of Block Lock Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr13",
     13,
     13,
     0,
     10563,
     "hsmcpcs Ch#3 High BER event Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr14",
     14,
     14,
     0,
     10564,
     "hsmcpcs Ch#3 Error Block Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr10_flds = {
    13, interrupts0_intclr10_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat11_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10565,
     "lsmcpcs Ch #0 AN Done Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10566,
     "RS FEC #0 AM lost lane 0 Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10567,
     "RS FEC #0 AM lost lane 1 Interrupt status. Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10568,
     "RS FEC #0 AM lost lane 2 Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10569,
     "RS FEC #0 AM lost lane 3 Interrupt status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10570,
     "RS FEC #0 uncorrectable Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10571,
     "RS FEC #0 deskew lost Interrupt status. Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10572,
     "RS FEC #0 BER over threshold Interrupt status. Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10573,
     "FC FEC #4 block lock gained Interrupt status. Interrupt Status"},

    {"stat9",
     9,
     9,
     0,
     10574,
     "FC FEC #4 Block lock lost Interrupt status. Interrupt Status"},

    {"stat10",
     10,
     10,
     0,
     10575,
     "FC FEC #4 Uncorrectable Codeword received Interrupt status. Interrupt "
     "Status"},

    {"stat11",
     11,
     11,
     0,
     10576,
     "FC FEC #4 Bad Codeword Received Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat11_flds = {
    12, interrupts0_intstat11_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr11_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10577,
     "lsmcpcs Ch #0 AN Done Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10578,
     "RS FEC #0 AM lost lane 0 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10579,
     "RS FEC #0 AM lost lane 1 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10580,
     "RS FEC #0 AM lost lane 2 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10581,
     "RS FEC #0 AM lost lane 3 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10582,
     "RS FEC #0 uncorrectable Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr6",
     6,
     6,
     0,
     10583,
     "RS FEC #0 deskew lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10584,
     "RS FEC #0 BER over threshold Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10585,
     "FC FEC #4 block lock gained Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr9",
     9,
     9,
     0,
     10586,
     "FC FEC #4 Block lock lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr10",
     10,
     10,
     0,
     10587,
     "FC FEC #4 Uncorrectable Codeword received Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr11",
     11,
     11,
     0,
     10588,
     "FC FEC #4 Bad Codeword Received Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr11_flds = {
    12, interrupts0_intclr11_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat12_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10589,
     "lsmcpcs Ch #1 AN Done Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10590,
     "RS FEC #1 AM lost lane 0 Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10591,
     "RS FEC #1 AM lost lane 1 Interrupt status. Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10592,
     "RS FEC #1 AM lost lane 2 Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10593,
     "RS FEC #1 AM lost lane 3 Interrupt status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10594,
     "RS FEC #1 uncorrectable Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10595,
     "RS FEC #1 deskew lost Interrupt status. Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10596,
     "RS FEC #1 BER over threshold Interrupt status. Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10597,
     "FC FEC #5 block lock gained Interrupt status. Interrupt Status"},

    {"stat9",
     9,
     9,
     0,
     10598,
     "FC FEC #5 Block lock lost Interrupt status. Interrupt Status"},

    {"stat10",
     10,
     10,
     0,
     10599,
     "FC FEC #5 Uncorrectable Codeword received Interrupt status. Interrupt "
     "Status"},

    {"stat11",
     11,
     11,
     0,
     10600,
     "FC FEC #5 Bad Codeword Received Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat12_flds = {
    12, interrupts0_intstat12_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr12_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10601,
     "lsmcpcs Ch #1 AN Done Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10602,
     "RS FEC #1 AM lost lane 0 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10603,
     "RS FEC #1 AM lost lane 1 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10604,
     "RS FEC #1 AM lost lane 2 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10605,
     "RS FEC #1 AM lost lane 3 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10606,
     "RS FEC #1 uncorrectable Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr6",
     6,
     6,
     0,
     10607,
     "RS FEC #1 deskew lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10608,
     "RS FEC #1 BER over threshold Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10609,
     "FC FEC #5 block lock gained Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr9",
     9,
     9,
     0,
     10610,
     "FC FEC #5 Block lock lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr10",
     10,
     10,
     0,
     10611,
     "FC FEC #5 Uncorrectable Codeword received Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr11",
     11,
     11,
     0,
     10612,
     "FC FEC #5 Bad Codeword Received Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr12_flds = {
    12, interrupts0_intclr12_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat13_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10613,
     "lsmcpcs Ch #2 AN Done Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10614,
     "RS FEC #2 AM lost lane 0 Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10615,
     "RS FEC #2 AM lost lane 1 Interrupt status. Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10616,
     "RS FEC #2 AM lost lane 2 Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10617,
     "RS FEC #2 AM lost lane 3 Interrupt status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10618,
     "RS FEC #2 uncorrectable Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10619,
     "RS FEC #2 deskew lost Interrupt status. Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10620,
     "RS FEC #2 BER over threshold Interrupt status. Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10621,
     "FC FEC #6 block lock gained Interrupt status. Interrupt Status"},

    {"stat9",
     9,
     9,
     0,
     10622,
     "FC FEC #6 Block lock lost Interrupt status. Interrupt Status"},

    {"stat10",
     10,
     10,
     0,
     10623,
     "FC FEC #6 Uncorrectable Codeword received Interrupt status. Interrupt "
     "Status"},

    {"stat11",
     11,
     11,
     0,
     10624,
     "FC FEC #6 Bad Codeword Received Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat13_flds = {
    12, interrupts0_intstat13_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr13_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10625,
     "lsmcpcs Ch #2 AN Done Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10626,
     "RS FEC #2 AM lost lane 0 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10627,
     "RS FEC #2 AM lost lane 1 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10628,
     "RS FEC #2 AM lost lane 2 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10629,
     "RS FEC #2 AM lost lane 3 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10630,
     "RS FEC #2 uncorrectable Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr6",
     6,
     6,
     0,
     10631,
     "RS FEC #2 deskew lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10632,
     "RS FEC #2 BER over threshold Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10633,
     "FC FEC #6 block lock gained Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr9",
     9,
     9,
     0,
     10634,
     "FC FEC #6 Block lock lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr10",
     10,
     10,
     0,
     10635,
     "FC FEC #6 Uncorrectable Codeword received Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr11",
     11,
     11,
     0,
     10636,
     "FC FEC #6 Bad Codeword Received Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr13_flds = {
    12, interrupts0_intclr13_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat14_fld_list[] = {
    {"stat0",
     0,
     0,
     0,
     10637,
     "lsmcpcs Ch #3 AN Done Interrupt status. Interrupt Status"},

    {"stat1",
     1,
     1,
     0,
     10638,
     "RS FEC #3 AM lost lane 0 Interrupt status. Interrupt Status"},

    {"stat2",
     2,
     2,
     0,
     10639,
     "RS FEC #3 AM lost lane 1 Interrupt status. Interrupt Status"},

    {"stat3",
     3,
     3,
     0,
     10640,
     "RS FEC #3 AM lost lane 2 Interrupt status. Interrupt Status"},

    {"stat4",
     4,
     4,
     0,
     10641,
     "RS FEC #3 AM lost lane 3 Interrupt status. Interrupt Status"},

    {"stat5",
     5,
     5,
     0,
     10642,
     "RS FEC #3 uncorrectable Interrupt status. Interrupt Status"},

    {"stat6",
     6,
     6,
     0,
     10643,
     "RS FEC #3 deskew lost Interrupt status. Interrupt Status"},

    {"stat7",
     7,
     7,
     0,
     10644,
     "RS FEC #3 BER over threshold Interrupt status. Interrupt Status"},

    {"stat8",
     8,
     8,
     0,
     10645,
     "FC FEC #7 block lock gained Interrupt status. Interrupt Status"},

    {"stat9",
     9,
     9,
     0,
     10646,
     "FC FEC #7 Block lock lost Interrupt status. Interrupt Status"},

    {"stat10",
     10,
     10,
     0,
     10647,
     "FC FEC #7 Uncorrectable Codeword received Interrupt status. Interrupt "
     "Status"},

    {"stat11",
     11,
     11,
     0,
     10648,
     "FC FEC #7 Bad Codeword Received Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat14_flds = {
    12, interrupts0_intstat14_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr14_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10649,
     "lsmcpcs Ch #3 AN Done Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr1",
     1,
     1,
     0,
     10650,
     "RS FEC #3 AM lost lane 0 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr2",
     2,
     2,
     0,
     10651,
     "RS FEC #3 AM lost lane 1 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr3",
     3,
     3,
     0,
     10652,
     "RS FEC #3 AM lost lane 2 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr4",
     4,
     4,
     0,
     10653,
     "RS FEC #3 AM lost lane 3 Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr5",
     5,
     5,
     0,
     10654,
     "RS FEC #3 uncorrectable Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr6",
     6,
     6,
     0,
     10655,
     "RS FEC #3 deskew lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr7",
     7,
     7,
     0,
     10656,
     "RS FEC #3 BER over threshold Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10657,
     "FC FEC #7 block lock gained Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

    {"clr9",
     9,
     9,
     0,
     10658,
     "FC FEC #7 Block lock lost Interrupt status clear. Clear Interrupt Status "
     "Register (if interrupt is level must also be cleared at source)"},

    {"clr10",
     10,
     10,
     0,
     10659,
     "FC FEC #7 Uncorrectable Codeword received Interrupt status clear. Clear "
     "Interrupt Status Register (if interrupt is level must also be cleared at "
     "source)"},

    {"clr11",
     11,
     11,
     0,
     10660,
     "FC FEC #7 Bad Codeword Received Interrupt status clear. Clear Interrupt "
     "Status Register (if interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr14_flds = {
    12, interrupts0_intclr14_fld_list, 16};

reg_decoder_fld_t interrupts0_intstat15_fld_list[] = {
    {"stat7", 7, 7, 0, 10661, "BPAN Ch#0 Interrupt status. Interrupt Status"},

    {"stat8", 8, 8, 0, 10662, "BPAN Ch#1 Interrupt status. Interrupt Status"},

    {"stat9", 9, 9, 0, 10663, "BPAN Ch#2 Interrupt status. Interrupt Status"},

    {"stat10",
     10,
     10,
     0,
     10664,
     "BPAN Ch#3 Interrupt status. Interrupt Status"},

};
reg_decoder_t interrupts0_intstat15_flds = {
    4, interrupts0_intstat15_fld_list, 16};

reg_decoder_fld_t interrupts0_intclr15_fld_list[] = {
    {"clr7",
     7,
     7,
     0,
     10665,
     "BPAN Ch#0 Interrupt status clear. Clear Interrupt Status Register (if "
     "interrupt is level must also be cleared at source)"},

    {"clr8",
     8,
     8,
     0,
     10666,
     "BPAN Ch#1 Interrupt status clear. Clear Interrupt Status Register (if "
     "interrupt is level must also be cleared at source)"},

    {"clr9",
     9,
     9,
     0,
     10667,
     "BPAN Ch#2 Interrupt status clear. Clear Interrupt Status Register (if "
     "interrupt is level must also be cleared at source)"},

    {"clr10",
     10,
     10,
     0,
     10668,
     "BPAN Ch#3 Interrupt status clear. Clear Interrupt Status Register (if "
     "interrupt is level must also be cleared at source)"},

};
reg_decoder_t interrupts0_intclr15_flds = {
    4, interrupts0_intclr15_fld_list, 16};

reg_decoder_fld_t glbl_version_fld_list[] = {
    {"version", 15, 0, 0, 10669, "Release version number"},

};
reg_decoder_t glbl_version_flds = {1, glbl_version_fld_list, 16};

reg_decoder_fld_t glbl_lnkstatovrd_fld_list[] = {
    {"rxchsyncovrd0",
     0,
     0,
     0,
     10670,
     "When set, overrides the Ch#0 Link Status from PCS (HS/LS). For "
     "diagnostics only "},

    {"rxchsyncovrd1",
     1,
     1,
     0,
     10671,
     "When set, overrides the Ch#1 Link Status from PCS (HS/LS). For "
     "diagnostics only "},

    {"rxchsyncovrd2",
     2,
     2,
     0,
     10672,
     "When set, overrides the Ch#2 Link Status from PCS (HS/LS). For "
     "diagnostics only "},

    {"rxchsyncovrd3",
     3,
     3,
     0,
     10673,
     "When set, overrides the Ch#3 Link Status from PCS (HS/LS). For "
     "diagnostics only "},

};
reg_decoder_t glbl_lnkstatovrd_flds = {4, glbl_lnkstatovrd_fld_list, 16};

reg_decoder_fld_t glbl_livelnkstat0_fld_list[] = {
    {"chlinkup0",
     0,
     0,
     0,
     10674,
     "Channel 0 Link is Up when this bit = 1. This is set when the PCS has "
     "obtained Sync and the MAC is not in Fault Condition."},

    {"chlinkup1",
     1,
     1,
     0,
     10675,
     "Channel 1 Link is Up when this bit = 1. This is set when the PCS has "
     "obtained Sync and the MAC is not in Fault Condition."},

    {"chlinkup2",
     2,
     2,
     0,
     10676,
     "Channel 2 Link is Up when this bit = 1.  This is set when the PCS has "
     "obtained Sync and the MAC is not in Fault Condition."},

    {"chlinkup3",
     3,
     3,
     0,
     10677,
     "Channel 3 Link is Up when this bit = 1.  This is set when the PCS has "
     "obtained Sync and the MAC is not in Fault Condition."},

    {"chmacflt0",
     4,
     4,
     0,
     10678,
     "Channel#0 MAC's Receive Fault is on when this bit = 1."},

    {"chmacflt1",
     5,
     5,
     0,
     10679,
     "Channel#1 MAC's Receive Fault is on when this bit = 1."},

    {"chmacflt2",
     6,
     6,
     0,
     10680,
     "Channel#2 MAC's Receive Fault is on when this bit = 1."},

    {"chmacflt3",
     7,
     7,
     0,
     10681,
     "Channel#3 MAC's Receive Fault is on when this bit = 1."},

    {"chsigstat0",
     8,
     8,
     0,
     10682,
     "SerDes Lane 0 Signal OK when this bit = 1. "},

    {"chsigstat1",
     9,
     9,
     0,
     10683,
     "SerDes Lane 1 Signal OK when this bit = 1. "},

    {"chsigstat2",
     10,
     10,
     0,
     10684,
     "SerDes Lane 2 Signal OK when this bit = 1. "},

    {"chsigstat3",
     11,
     11,
     0,
     10685,
     "SerDes Lane 3 Signal OK when this bit = 1. "},

    {"txidle0", 12, 12, 0, 10686, "Channel 0 Tx Idle  when this bit = 1. "},

    {"txidle1", 13, 13, 0, 10687, "Channel 1 Tx Idle when this bit = 1. "},

    {"txidle2", 14, 14, 0, 10688, "Channel 2 Tx Idle when this bit = 1. "},

    {"txidle3", 15, 15, 0, 10689, "Channel 3 Tx Idle when this bit = 1. "},

};
reg_decoder_t glbl_livelnkstat0_flds = {16, glbl_livelnkstat0_fld_list, 16};

reg_decoder_fld_t glbl_spare0_fld_list[] = {
    {"spare0", 15, 0, 0, 10690, "Spare Register 0: SW RW, HW RO"},

};
reg_decoder_t glbl_spare0_flds = {1, glbl_spare0_fld_list, 16};

reg_decoder_fld_t glbl_spare2_fld_list[] = {
    {"spare2", 15, 0, 0, 10691, "Spare Register 2: SW RW, HW RO"},

};
reg_decoder_t glbl_spare2_flds = {1, glbl_spare2_fld_list, 16};

reg_decoder_fld_t glbl_ch0mode_fld_list[] = {
    {"speed",
     5,
     2,
     0,
     10692,
     "4'b1111-4'b1110: Reserved for Future Use 4'b1101: 5G 4'b1100: 2.5G "
     "4'b1011 : 400G 4'b1010 : 200G 4'b1001 : 100G 4'b1000 : 50G 4'b0111 : 40G "
     "4'b0110 : 25G 4'b0101 : 20G 4'b0100 : 10G 4'b0011 : 1G 4'b0010 : 100M "
     "4'b0001 : 10M 4'b0000 : Disabled"},

    {"swreset", 1, 1, 0, 10693, "1'b1 Reset Active 1'b0 Normal Operation"},

    {"chena", 0, 0, 0, 10694, "1'b1 Normal Operation 1'b0 Channel Disabled"},

};
reg_decoder_t glbl_ch0mode_flds = {3, glbl_ch0mode_fld_list, 16};

reg_decoder_fld_t glbl_ch1mode_fld_list[] = {
    {"speed",
     5,
     2,
     0,
     10695,
     "4'b1111-4'b1110: Reserved for Future Use 4'b1101: 5G 4'b1100: 2.5G "
     "4'b1011 : 400G 4'b1010 : 200G 4'b1001 : 100G 4'b1000 : 50G 4'b0111 : 40G "
     "4'b0110 : 25G 4'b0101 : 20G 4'b0100 : 10G 4'b0011 : 1G 4'b0010 : 100M "
     "4'b0001 : 10M 4'b0000 : Disabled"},

    {"swreset", 1, 1, 0, 10696, "1'b1 Reset Active 1'b0 Normal Operation"},

    {"chena", 0, 0, 0, 10697, "1'b1 Normal Operation 1'b0 Channel Disabled"},

};
reg_decoder_t glbl_ch1mode_flds = {3, glbl_ch1mode_fld_list, 16};

reg_decoder_fld_t glbl_ch2mode_fld_list[] = {
    {"speed",
     5,
     2,
     0,
     10698,
     "4'b1111-4'b1110: Reserved for Future Use 4'b1101: 5G 4'b1100: 2.5G "
     "4'b1011 : 400G 4'b1010 : 200G 4'b1001 : 100G 4'b1000 : 50G 4'b0111 : 40G "
     "4'b0110 : 25G 4'b0101 : 20G 4'b0100 : 10G 4'b0011 : 1G 4'b0010 : 100M "
     "4'b0001 : 10M 4'b0000 : Disabled"},

    {"swreset", 1, 1, 0, 10699, "1'b1 Reset Active 1'b0 Normal Operation"},

    {"chena", 0, 0, 0, 10700, "1'b1 Normal Operation 1'b0 Channel Disabled"},

};
reg_decoder_t glbl_ch2mode_flds = {3, glbl_ch2mode_fld_list, 16};

reg_decoder_fld_t glbl_ch3mode_fld_list[] = {
    {"speed",
     5,
     2,
     0,
     10701,
     "4'b1111-4'b1110: Reserved for Future Use 4'b1101: 5G 4'b1100: 2.5G "
     "4'b1011 : 400G 4'b1010 : 200G 4'b1001 : 100G 4'b1000 : 50G 4'b0111 : 40G "
     "4'b0110 : 25G 4'b0101 : 20G 4'b0100 : 10G 4'b0011 : 1G 4'b0010 : 100M "
     "4'b0001 : 10M 4'b0000 : Disabled"},

    {"swreset", 1, 1, 0, 10702, "1'b1 Reset Active 1'b0 Normal Operation"},

    {"chena", 0, 0, 0, 10703, "1'b1 Normal Operation 1'b0 Channel Disabled"},

};
reg_decoder_t glbl_ch3mode_flds = {3, glbl_ch3mode_fld_list, 16};

reg_decoder_fld_t glbl_fint0_fld_list[] = {
    {"ovrd0int15", 15, 15, 0, 10704, "Mcmac Ch#3 txidle interrupt override"},

    {"ovrd0int14", 14, 14, 0, 10705, "Mcmac Ch#2 txidle interrupt override"},

    {"ovrd0int13", 13, 13, 0, 10706, "Mcmac Ch#1 txidle interrupt override"},

    {"ovrd0int12", 12, 12, 0, 10707, "Mcmac Ch#0 txidle interrupt override"},

    {"ovrd0int11", 11, 11, 0, 10708, "Rx SigOK Lane#3 interrupt override"},

    {"ovrd0int10", 10, 10, 0, 10709, "Rx SigOK Lane#2 interrupt override"},

    {"ovrd0int9", 9, 9, 0, 10710, "Rx SigOK Lane#1 interrupt override"},

    {"ovrd0int8", 8, 8, 0, 10711, "Rx SigOK Lane#0 interrupt override"},

    {"ovrd0int7", 7, 7, 0, 10712, "Mcmac Ch#3 rx fault interrupt override"},

    {"ovrd0int6", 6, 6, 0, 10713, "Mcmac Ch#2 rx fault interrupt override"},

    {"ovrd0int5", 5, 5, 0, 10714, "Mcmac Ch#1 rx fault interrupt override"},

    {"ovrd0int4", 4, 4, 0, 10715, "Mcmac Ch#0 rx fault interrupt override"},

    {"ovrd0int3", 3, 3, 0, 10716, "Fifoctrl Ch#3 RX sync Interrupt override"},

    {"ovrd0int2", 2, 2, 0, 10717, "Fifoctrl Ch#2 RX sync Interrupt override"},

    {"ovrd0int1", 1, 1, 0, 10718, "Fifoctrl Ch#1 RX sync Interrupt override"},

    {"ovrd0int0", 0, 0, 0, 10719, "Fifoctrl Ch#0 RX sync Interrupt override"},

};
reg_decoder_t glbl_fint0_flds = {16, glbl_fint0_fld_list, 16};

reg_decoder_fld_t glbl_defineid_fld_list[] = {
    {"defineid", 15, 0, 0, 10720, "Captures the compiler directives used"},

};
reg_decoder_t glbl_defineid_flds = {1, glbl_defineid_fld_list, 16};

reg_decoder_fld_t glbl_productid_fld_list[] = {
    {"productid", 15, 0, 0, 10721, "Product ID"},

};
reg_decoder_t glbl_productid_flds = {1, glbl_productid_fld_list, 16};

reg_decoder_fld_t stats0_swreset_fld_list[] = {
    {"swreset0", 0, 0, 0, 10722, "Software Reset for Stats Ch#0"},

    {"swreset1", 1, 1, 0, 10723, "Software Reset for Stats Ch#1"},

    {"swreset2", 2, 2, 0, 10724, "Software Reset for Stats Ch#2"},

    {"swreset3", 3, 3, 0, 10725, "Software Reset for Stats Ch#3"},

};
reg_decoder_t stats0_swreset_flds = {4, stats0_swreset_fld_list, 16};

reg_decoder_fld_t stats0_rdctrl_fld_list[] = {
    {"cntrnum",
     6,
     0,
     0,
     10726,
     "This field defines the Statistics Counter to be read for the specified "
     "Channel (Statistics Channel to Read). Once the Read operation is "
     "completed the 64-bit Counter value is loaded into UMAC Statistics Read "
     "Data Registers. The following are counters and their offsets (in Hex) "
     "implemented in the UMAC: 00: Frames Received O.K 01: Frames Received All "
     "(Good/Bad Frames) 02: Frames Received with FCS Error 03: Frames with any "
     "Error (CRC, Length, Alignment Error). 04: Octets Received in Good "
     "Frames. 05: Octets Received (Good/Bad Frames) 06: Frames Received with "
     "Unicast Addresses. 07: Frames Received with Multicast Addresses. 08: "
     "Frames Received with Broadcast Addresses. 09: Frames Received of type "
     "PAUSE. 10: Frames Received with Length Error. 11: Frames Received "
     "Undersized (No Error) 12: Frames Received Oversized (No Error) 13: "
     "Fragments Received 14: Jabber Received 15: Priority Pause Frames. 16: "
     "CRC Error (Stomped) 17: Frame Too Long. 18: Rx VLAN Frames (Good). 19: "
     "Frames Dropped (Buffer Full) 20: Frames Received Length<64 21: Frames "
     "Received Length=64. 22: Frames Received Length=65-127. 23: Frames "
     "Received Length=128-255. 24: Frames Received Length=256-511. 25: Frames "
     "Received Length=512-1023. 26: Frames Received Length=1024-1518. 27: "
     "Frames Received Length=1519-2047.   28: Frames Received "
     "Length=2048-4095.   29: Frames Received Length=4096-8191.   30: Frames "
     "Received Length=8192-9215.   31: Frames Received Length>=9216. 32: "
     "Frames Transmitted O.K 33: Frames Transmitted All (Good/Bad Frames) 34: "
     "Frames Transmitted with Error 35: Octets Transmitted with out error 36: "
     "Octets Transmitted Total (Good/Error) 37: Frames Transmitted Unicast 38: "
     "Frames Transmitted Multicast 39: Frames Transmitted Broadcast 40: Frames "
     "Transmitted Pause 41: Frames Transmitted PriPause 42: Frames Transmitted "
     "VLAN. 43: Frames Transmitted Length<64 44: Frames Transmitted Length=64. "
     "45: Frames Transmitted Length=65-127. 46: Frames Transmitted "
     "Length=128-255. 47: Frames Transmitted Length=256-511. 48: Frames "
     "Transmitted Length=512-1023. 49: Frames Transmitted Length=1024-1518. "
     "50: Frames Transmitted Length=1519-2047.   51: Frames Transmitted "
     "Length=2048-4095.   52: Frames Transmitted Length=4096-8191.   53: "
     "Frames Transmitted Length=8192-9215.   54: Frames Transmitted "
     "Length>=9216. 55: Pri#0 Frames Transmitted. 56: Pri#1 Frames "
     "Transmitted. 57: Pri#2 Frames Transmitted. 58: Pri#3 Frames Transmitted. "
     "59: Pri#4 Frames Transmitted. 60: Pri#5 Frames Transmitted. 61: Pri#6 "
     "Frames Transmitted. 62: Pri#7 Frames Transmitted. 63: Pri#0 Frames "
     "Received. 64: Pri#1 Frames Received. 65: Pri#2 Frames Received. 66: "
     "Pri#3 Frames Received. 67: Pri#4 Frames Received. 68: Pri#5 Frames "
     "Received. 69: Pri#6 Frames Received. 70: Pri#7 Frames Received. 71: "
     "Transmit Pri#0 Pause 1US Count. 72: Transmit Pri#1 Pause 1US Count. 73: "
     "Transmit Pri#2 Pause 1US Count. 74: Transmit Pri#3 Pause 1US Count. 75: "
     "Transmit Pri#4 Pause 1US Count. 76: Transmit Pri#5 Pause 1US Count. 77: "
     "Transmit Pri#6 Pause 1US Count. 78: Transmit Pri#7 Pause 1US Count. 79: "
     "Receive Pri#0 Pause 1US Count. 80: Receive Pri#1 Pause 1US Count. 81: "
     "Receive Pri#2 Pause 1US Count. 82: Receive Pri#3 Pause 1US Count. 83: "
     "Receive Pri#4 Pause 1US Count. 84: Receive Pri#5 Pause 1US Count. 85: "
     "Receive Pri#6 Pause 1US Count. 86: Receive Pri#7 Pause 1US Count. 87: "
     "Receive Standard Pause 1US Count. 88: Frames Truncated."},

    {"channum",
     8,
     7,
     0,
     10727,
     "This field defines the Channel from which the Statistics are to be "
     "read."},

    {"cor",
     13,
     13,
     0,
     10728,
     "When this bit is set, the Contents of the Counter are cleared after the "
     "Read is performed. This should be set only when doing non-priority "
     "reads. The COR read function is ignored when the 'Priority Read' bit is "
     "also set."},

    {"pr",
     14,
     14,
     0,
     10729,
     "When this bit is set, the read access to the STAT Memory will have "
     "Priority over the normal STAT Memory Background Operation. When this bit "
     "is clear, the read access to the STAT memory will be merged with the "
     "STAT memory back ground operation."},

    {"rsc",
     15,
     15,
     0,
     10730,
     "This is set by Software for the UMAC to read a statistics counter. When "
     "this bit is set, the UMAC accesses internal memory and reads the "
     "specified counter. Once the read operation completes, the bit is cleared "
     "by the UMAC. The following are the steps for reading the Statistics "
     "Counter. 1) Read and Verify this bit is cleared. If not cleared wait "
     "until the bit is cleared (polling) 2) Program the Counter to access and "
     "set this bit to 1'b1. 3) Poll this register until the bit is cleared. 4) "
     "The counter information is available in the X-UMAC Statistics Read Data "
     "Registers."},

};
reg_decoder_t stats0_rdctrl_flds = {5, stats0_rdctrl_fld_list, 16};

reg_decoder_fld_t stats0_rdata0_fld_list[] = {
    {"rdata",
     15,
     0,
     0,
     10731,
     "This contains the bits [15:0] of the Statistics Counter that is read."},

};
reg_decoder_t stats0_rdata0_flds = {1, stats0_rdata0_fld_list, 16};

reg_decoder_fld_t stats0_rdata1_fld_list[] = {
    {"rdata",
     15,
     0,
     0,
     10732,
     "This contains the bits [31:16] of the Statistics Counter that is read."},

};
reg_decoder_t stats0_rdata1_flds = {1, stats0_rdata1_fld_list, 16};

reg_decoder_fld_t stats0_rdata2_fld_list[] = {
    {"rdata",
     15,
     0,
     0,
     10733,
     "This contains the bits [47:32] of the Statistics Counter that is read."},

};
reg_decoder_t stats0_rdata2_flds = {1, stats0_rdata2_fld_list, 16};

reg_decoder_fld_t stats0_rdata3_fld_list[] = {
    {"rdata",
     15,
     0,
     0,
     10734,
     "This contains the bits [63:48] of the Statistics Counter that is read."},

};
reg_decoder_t stats0_rdata3_flds = {1, stats0_rdata3_fld_list, 16};

reg_decoder_fld_t stats0_statsrst_fld_list[] = {
    {"clr0",
     0,
     0,
     0,
     10735,
     "Setting this field will start the initialization (resetting) of all the "
     "Statistic Counters in UMAC for Channel#N. The actual initialization "
     "process starts after this bit is cleared."},

    {"clr1",
     1,
     1,
     0,
     10736,
     "Setting this field will start the initialization (resetting) of all the "
     "Statistic Counters in UMAC for Channel#N. The actual initialization "
     "process starts after this bit is cleared."},

    {"clr2",
     2,
     2,
     0,
     10737,
     "Setting this field will start the initialization (resetting) of all the "
     "Statistic Counters in UMAC for Channel#N. The actual initialization "
     "process starts after this bit is cleared."},

    {"clr3",
     3,
     3,
     0,
     10738,
     "Setting this field will start the initialization (resetting) of all the "
     "Statistic Counters in UMAC for Channel#N. The actual initialization "
     "process starts after this bit is cleared."},

};
reg_decoder_t stats0_statsrst_flds = {4, stats0_statsrst_fld_list, 16};

reg_decoder_fld_t stats0_ledinfo0_fld_list[] = {
    {"txactive",
     0,
     0,
     0,
     10739,
     "When set, indicates transmitted a frame since the register last read "},

    {"txerror",
     1,
     1,
     0,
     10740,
     "When set, indicates transmitted an Error Frame (EOF_ERROR, UNDERRUN, "
     "JABBER) since the register last read "},

    {"txunderrun",
     2,
     2,
     0,
     10741,
     "When set, indicates underflow happened since the register last read "},

    {"txpause",
     3,
     3,
     0,
     10742,
     "When set, indicates trasnsmitted a Pause Frame since the register last "
     "read "},

    {"rxactive",
     4,
     4,
     0,
     10743,
     "When set, indicates Received a frame since the register last read "},

    {"rxcrcerror",
     5,
     5,
     0,
     10744,
     "When set, indicates Received a frame with CRC Error since the register "
     "last read "},

    {"rxerror",
     6,
     6,
     0,
     10745,
     "When set, indicates Received a frame with error since the register last "
     "read "},

    {"rxoverflow",
     7,
     7,
     0,
     10746,
     "When set, indicates overflow happend since the register last read "},

    {"rxpause",
     8,
     8,
     0,
     10747,
     "When set, indicates Received a pause frame since the register last "
     "read "},

};
reg_decoder_t stats0_ledinfo0_flds = {9, stats0_ledinfo0_fld_list, 16};

reg_decoder_fld_t stats0_ledinfo1_fld_list[] = {
    {"txactive",
     0,
     0,
     0,
     10748,
     "When set, indicates transmitted a frame since the register last read "},

    {"txerror",
     1,
     1,
     0,
     10749,
     "When set, indicates transmitted an Error Frame (EOF_ERROR, UNDERRUN, "
     "JABBER) since the register last read "},

    {"txunderrun",
     2,
     2,
     0,
     10750,
     "When set, indicates underflow happened since the register last read "},

    {"txpause",
     3,
     3,
     0,
     10751,
     "When set, indicates trasnsmitted a Pause Frame since the register last "
     "read "},

    {"rxactive",
     4,
     4,
     0,
     10752,
     "When set, indicates Received a frame since the register last read "},

    {"rxcrcerror",
     5,
     5,
     0,
     10753,
     "When set, indicates Received a frame with CRC Error since the register "
     "last read "},

    {"rxerror",
     6,
     6,
     0,
     10754,
     "When set, indicates Received a frame with error since the register last "
     "read "},

    {"rxoverflow",
     7,
     7,
     0,
     10755,
     "When set, indicates overflow happend since the register last read "},

    {"rxpause",
     8,
     8,
     0,
     10756,
     "When set, indicates Received a pause frame since the register last "
     "read "},

};
reg_decoder_t stats0_ledinfo1_flds = {9, stats0_ledinfo1_fld_list, 16};

reg_decoder_fld_t stats0_ledinfo2_fld_list[] = {
    {"txactive",
     0,
     0,
     0,
     10757,
     "When set, indicates transmitted a frame since the register last read "},

    {"txerror",
     1,
     1,
     0,
     10758,
     "When set, indicates transmitted an Error Frame (EOF_ERROR, UNDERRUN, "
     "JABBER) since the register last read "},

    {"txunderrun",
     2,
     2,
     0,
     10759,
     "When set, indicates underflow happened since the register last read "},

    {"txpause",
     3,
     3,
     0,
     10760,
     "When set, indicates trasnsmitted a Pause Frame since the register last "
     "read "},

    {"rxactive",
     4,
     4,
     0,
     10761,
     "When set, indicates Received a frame since the register last read "},

    {"rxcrcerror",
     5,
     5,
     0,
     10762,
     "When set, indicates Received a frame with CRC Error since the register "
     "last read "},

    {"rxerror",
     6,
     6,
     0,
     10763,
     "When set, indicates Received a frame with error since the register last "
     "read "},

    {"rxoverflow",
     7,
     7,
     0,
     10764,
     "When set, indicates overflow happend since the register last read "},

    {"rxpause",
     8,
     8,
     0,
     10765,
     "When set, indicates Received a pause frame since the register last "
     "read "},

};
reg_decoder_t stats0_ledinfo2_flds = {9, stats0_ledinfo2_fld_list, 16};

reg_decoder_fld_t stats0_ledinfo3_fld_list[] = {
    {"txactive",
     0,
     0,
     0,
     10766,
     "When set, indicates transmitted a frame since the register last read "},

    {"txerror",
     1,
     1,
     0,
     10767,
     "When set, indicates transmitted an Error Frame (EOF_ERROR, UNDERRUN, "
     "JABBER) since the register last read "},

    {"txunderrun",
     2,
     2,
     0,
     10768,
     "When set, indicates underflow happened since the register last read "},

    {"txpause",
     3,
     3,
     0,
     10769,
     "When set, indicates trasnsmitted a Pause Frame since the register last "
     "read "},

    {"rxactive",
     4,
     4,
     0,
     10770,
     "When set, indicates Received a frame since the register last read "},

    {"rxcrcerror",
     5,
     5,
     0,
     10771,
     "When set, indicates Received a frame with CRC Error since the register "
     "last read "},

    {"rxerror",
     6,
     6,
     0,
     10772,
     "When set, indicates Received a frame with error since the register last "
     "read "},

    {"rxoverflow",
     7,
     7,
     0,
     10773,
     "When set, indicates overflow happend since the register last read "},

    {"rxpause",
     8,
     8,
     0,
     10774,
     "When set, indicates Received a pause frame since the register last "
     "read "},

};
reg_decoder_t stats0_ledinfo3_flds = {9, stats0_ledinfo3_fld_list, 16};

reg_decoder_fld_t stats0_sts_fld_list[] = {
    {"ch0initdone",
     0,
     0,
     0,
     10775,
     "This is set when the Internal initialization of stat counters are "
     "complete for Channel#0 after Powerup/SwReset or after the Software "
     "controlled Counter Clear Operation. Software should poll this bit before "
     "determining the completion of the Counter Clear operation."},

    {"ch1initdone",
     1,
     1,
     0,
     10776,
     "This is set when the Internal initialization of stat counters are "
     "complete for Channel#1 after Powerup/SwReset or after the Software "
     "controlled Counter Clear Operation. Software should poll this bit before "
     "determining the completion of the Counter Clear operation."},

    {"ch2initdone",
     2,
     2,
     0,
     10777,
     "This is set when the Internal initialization of stat counters are "
     "complete for Channel#2 after Powerup/SwReset or after the Software "
     "controlled Counter Clear Operation. Software should poll this bit before "
     "determining the completion of the Counter Clear operation."},

    {"ch3initdone",
     3,
     3,
     0,
     10778,
     "This is set when the Internal initialization of stat counters are "
     "complete for Channel#3 after Powerup/SwReset or after the Software "
     "controlled Counter Clear Operation. Software should poll this bit before "
     "determining the completion of the Counter Clear operation."},

};
reg_decoder_t stats0_sts_flds = {4, stats0_sts_fld_list, 16};

reg_decoder_fld_t mcmac0_ctrl_fld_list[] = {
    {"txenable",
     0,
     0,
     0,
     10779,
     "When set, this Channel's MAC transmitter is enabled and it will transmit "
     "frames from the Transmit FIFO/Application logic on to the PCS Interface. "
     "When reset, this Channel's MAC's transmitter is disabled and will not "
     "transmit any frames. "},

    {"rxenable",
     1,
     1,
     0,
     10780,
     "When set, this Channel's MAC receiver is enabled and it will receive "
     "frames from the PCS and transfer them to Receive FIFO/Application logic. "
     "When reset, this Channel's MAC Core receiver is disabled and will not "
     "receive any frames."},

    {"swreset",
     2,
     2,
     0,
     10781,
     "Setting this bit resets this Channel's various MAC logic to the default "
     "state. The contents of the Configuration Registers related to this "
     "Channel are not affected by this bit.  This Channel's MAC logic is "
     "continued to be in reset state while this bit is set."},

    {"maclpbk",
     3,
     3,
     0,
     10782,
     "Setting this bit enables the Loopback on the MAC-PCS Interface for this "
     "Channel. The Transmit data and controls are looped back on to the "
     "receive MAC module for this Channel.  When this loopback is enabled, the "
     "data received from the PCS for this channel is ignored."},

    {"faultovrd",
     4,
     4,
     0,
     10783,
     "When set, overrides the Ch#0 Link Fault Status. For diagnostics only"},

    {"txdrn",
     5,
     5,
     0,
     10784,
     "Setting this bit causes transmit path to enter into Drain mode where all "
     "the data from the TXFIFO is drained out."},

};
reg_decoder_t mcmac0_ctrl_flds = {6, mcmac0_ctrl_fld_list, 16};

reg_decoder_fld_t mcmac0_txconfig_fld_list[] = {
    {"disfcs",
     0,
     0,
     0,
     10785,
     "When set, the FCS calculation and insertion logic is disabled in the "
     "transmit path for this Channel. FCS insertion is disabled for all the "
     "frames. When reset, the FCS is calculated on all frames and insertion is "
     "based on the per frame control signal (ff_txchdisfcs for this Channel).  "
     "When the Disable FCS Insertion is set, it is expected that the frames "
     "coming in from the Application logic will meet the 64-byte MinFrameSize "
     "requirements and contains the FCS field to make the outgoing frame IEEE "
     "802.3 specification Compliant."},

    {"invfcs",
     1,
     1,
     0,
     10786,
     "When set, the UMAC inverts the FCS field that is being inserted into the "
     "outgoing frames for this Channel. When reset, the UMAC Core operates "
     "normally for FCS insertion for this Channel.   Note: According to the "
     "IEEE 802.3 specification, the FCS is calculated beginning from the first "
     "byte of DA until the last byte of the DATA/PAD field. The calculated FCS "
     "is then inverted and sent out MSB first. So in normal mode, the "
     "calculated FCS is inverted before being inserted into the outgoing "
     "frame. When the Invert FCS is set, the FCS field is double inverted (No "
     "inversion happens)."},

    {"ifglength",
     7,
     2,
     0,
     10787,
     "This determines the minimum IFG (Inter Frame Gap) in Bytes, to be "
     "inserted between outgoing frames for this Channel, when the Application "
     "logic has back to back frames, and no Frame Pacing is defined. The IEEE "
     "802.3 specification specifies a minimum IFG of 96 bit-times between "
     "frames. The following are the permissible values for the IFG Length "
     "field.   For 10M, 100M, and 1000M, the range is as follows. Minimum: 1 "
     "Maximum: 63.  For 10G the range is as follows: Minimum: 8 Maximum: 63.  "
     "For 40G, 100G the range is as follows: Minimum: 12 Maximum: 63.  In case "
     "of 10G speed and above, the Deficit IDLE Counter is implemented to "
     "maintain the average rate of the programmed IFG.  The "
     "ff_txnextfrifg[7:0] value temporarily overrides the IFG programmed in "
     "this register.  When the IFG Length field is programmed with Zero, then "
     "the UMAC operates in Zero length IFG mode where the frames are "
     "transmitted back to back while horning the START alignment to Lane#0."},

    {"fcfrmgen",
     8,
     8,
     0,
     10788,
     "When set, this Channel is enabled for transmission of PAUSE Control "
     "Frames. The PAUSE Control frames are transmitted either on Software "
     "control or based on Application logic controls. The parameters for the "
     "generated PAUSE Frame are configured into the registers. When reset, "
     "this Channel does not generate a PAUSE Control Frame."},

    {"pfcfrmgen",
     9,
     9,
     0,
     10789,
     "When set, this Channel is enabled for transmission of PFC Control "
     "Frames. The PFC Control frames are transmitted based on Application "
     "logic's controls. When reset, this Channel doesn't generate priority "
     "PAUSE Control Frame."},

    {"prelength",
     13,
     11,
     0,
     10790,
     "This defines length of the Preamble that is used for every outgoing "
     "frame for this Channel.  Based on the MAC's operating speed as specified "
     "below:  10G Rates and above - 3'b000: Default 8-bytes (START, 6 bytes of "
     "PRE, 1 byte of SFD) - 3'b001: Reserved - 3'b010: Reserved - 3'b011: "
     "Reserved - 3'b100: bytes (START, 2 bytes of PRE, 1 byte of SFD). - "
     "3'b101: Reserved - 3'b111: Reserved - 3'b111: Reserved  1G Rates and "
     "below - 3'b000: Default 8-bytes (7 Bytes of PRE plus 1 Byte of SFD) - "
     "3'b001: 1 Byte of SFD only. - 3'b010: 1 Byte of PRE plus 1 Byte of SFD - "
     "3'b011: 2 Bytes of PRE plus 1 Byte of SFD - 3'b100: 3 Bytes of PRE plus "
     "1 Byte of SFD - 3'b101: 4 Bytes of PRE plus 1 Byte of SFD - 3'b110: 5 "
     "Bytes of PRE plus 1 Byte of SFD - 3'b111: 6 Bytes of PRE plus 1 Byte of "
     "SFD"},

    {"enfrmpace",
     14,
     14,
     0,
     10791,
     "Enable Pacing of Frames using the ff_txnextfrmifg value. When this is "
     "set, the value from ff_txnextfrmifg is used as the IFG between frames. "
     "When this is not set, the value programmed in IFG Length is used for all "
     "frames."},

    {"enautodrnonflt",
     15,
     15,
     0,
     10792,
     "Enables Auto Draining of Frames from FIFO Interface when the MAC "
     "receiver is in Fault state."},

};
reg_decoder_t mcmac0_txconfig_flds = {8, mcmac0_txconfig_fld_list, 16};

reg_decoder_fld_t mcmac0_rxconfig_fld_list[] = {
    {"disfcschk",
     0,
     0,
     0,
     10793,
     "When set, the UMAC doesn't perform FCS Checking on the incoming frames "
     "for this Channel and the CRC Error status is not set for any of the "
     "frames. When reset, the UMAC operates normally and checks FCS on every "
     "incoming frame, for this Channel. Frames with CRC Errors are reported in "
     "the Frame Status."},

    {"stripfcs",
     1,
     1,
     0,
     10794,
     "When set, the FCS field is stripped of the frame before the Frame is "
     "transferred to the Application logic for this Channel's frames. The "
     "Frame Length field is updated to reflect the new length (without the FCS "
     "field). When reset, the FCS field is not stripped and is part of the "
     "frame that is transferred to Application logic.   Irrespective of the "
     "Strip FCS field, FCS Checking is performed on every frame unless Disable "
     "FCS Check is set. When theFCS field is checked, the CRC Error status is "
     "reported for every frame. "},

    {"vlanchk",
     3,
     2,
     0,
     10795,
     "Per the IEEE 802.3 specification, the minFrameSize and maxFrameSize "
     "values for untagged frames are set to 64 bytes and 1518 bytes "
     "respectively. The values for tagged (VLAN Tagged) frame sizes are "
     "updated to account for 4-bytes of VLAN Tag and the maxFrameSize value is "
     "updated to 1522 Bytes.   The UMAC supports up to 3 VLAN Tags per frame. "
     "It can adjust the maxFrameSize field to support up to 3 VLAN Tags (12 "
     "Bytes), based on the number of VLAN tags that are present in the "
     "incoming frame. This helps in false MaxFrameSize violation reporting for "
     "frames with VLAN tags that exceed 1518 bytes but are under the "
     "1518+tagbytes in length.  The UMAC will use the value programmed in this "
     "field to determine the number of VLAN's for which the MAC will check "
     "(for this Channel) and update the MaxFrameSize field accordingly.  "
     "2'b00: Disable VLAN Checking. 2'b01: Check for up to 1 VLAN Tag (Use the "
     "VLAN Tag#1 Register) 2'b10: Check for up to 2 VLAN Tags (Use VLAN Tag#1 "
     "and #2 Registers) 2'b11: Check for up to 3 VLAN Tags (Use VLAN Tag#1, "
     "#2, and #3Registers)"},

    {"promiscuous",
     4,
     4,
     0,
     10796,
     "When set, the UMAC disables address checking on all frames that are "
     "received on this Channel, and all the frames received on this channel "
     "are passed onto the FIFO Interface. The Frame Status reflects the status "
     "of the Address Checking.  When the bit is cleared, the DA field is "
     "compared against the contents of the MAC Address Register (for Unicast "
     "Frames) and the Multicast Hash table (for Multicast Frames). Frames that "
     "fail topass the Address filtering will be dropped and not transferred to "
     "the FIFO Interface  "},

    {"enrxfcdec",
     5,
     5,
     0,
     10797,
     "When set, the UMAC Core is enabled for Flow-Control decode operation for "
     "this Channel and it will decode all the incoming frames for PAUSE "
     "Control Frames as specified in the IEEE 802.3 Specification.   If the "
     "UMAC Core receives a valid PAUSE Control Frame, it will disable the "
     "transmission of user data frames for the time given in the PAUSE_TIME "
     "field of the PAUSE Control Frame.  If the UMAC Core receives a valid "
     "Priority PAUSE Control Frame, it will load the timers and providethe "
     "XOFF indication to the Application logic (ff_rxch0pfcxoff[7:0]) based on "
     "the Time Vector fields in the 8 priorities.  When reset, the "
     "Flow-Control Operation in the UMAC is disabled for this Channel, and it "
     "does not decode the frames for PAUSE Control Frames or Priority Pause "
     "Flow Control Frames."},

    {"prelength",
     7,
     7,
     0,
     10798,
     "This defines the START/PREAMBLE/SFD length that is expected of all the "
     "frames received on this Channel.  1'b0: Default 8-bytes (START, 6 bytes "
     "of PREAMBLE, 1 byte of SFD) 1'b1: 4-bytes (START, 2 bytes of PREAMBLE, 1 "
     "byte of SFD).  This is applicable when the Channel is operating at 10G "
     "rates and above."},

    {"filterpf",
     8,
     8,
     0,
     10799,
     "When this bit is set, the UMAC will filter out the frames with DA "
     "matching the reserved Multicast PAUSE Control Frame address "
     "(01:80:C2:00:00:01) from being sent to the Application Logic.   When "
     "this bit is reset, the UMAC does not filter out the frames with the DA "
     "matching the reserved Multicast PAUSE Control Frame address "
     "(01:80:C2:00:00:01).  This bit has no effect on actual "
     "processing/decoding on the PAUSE (Priority PAUSE) Control frames, which "
     "is controlled thru theEnable Rx Flow Control Decode bit.  When the PAUSE "
     "control frames are passed to the application logic, (Filter PAUSE "
     "Frames) is not set, but the application logic can still identify the "
     "PAUSE Control frames using Bit[31] in ff_rxsts that is sent out with the "
     "frame."},

    {"enearlyeofdet",
     9,
     9,
     0,
     10800,
     "This should be set only when the UMAC is configured for 4x10G mode and "
     "the Low Latency option is selected in the PCS. This enables the "
     "detection of Early EOF indication from PCS which helps in reducing the "
     "latency.  Note: This is to be set only when the UMAC is operating in "
     "4x10G Mode. Setting this in other modes will cause the UMAC to have "
     "unexpected behavior."},

};
reg_decoder_t mcmac0_rxconfig_flds = {8, mcmac0_rxconfig_fld_list, 16};

reg_decoder_fld_t mcmac0_maxfrmsize_fld_list[] = {
    {"maxfrmsize",
     15,
     0,
     0,
     10801,
     "This field determines the Maximum Frame Size for untagged frames that is "
     "used in checking the MaxFrameLength violations on this Channel. For "
     "frames larger than this value, the MaxFrameLength Error bit is set in "
     "the Frame Status."},

};
reg_decoder_t mcmac0_maxfrmsize_flds = {1, mcmac0_maxfrmsize_fld_list, 16};

reg_decoder_fld_t mcmac0_maxtxjabsize_fld_list[] = {
    {"maxtxjabsize",
     15,
     0,
     0,
     10802,
     "This field determines the Jabber Size for the outgoing (Transmit) frames "
     "on this Channel. When the length of the current outgoing frame on this "
     "Channel exceeds the value programmed in this field, the Frame is "
     "considered a Jabber Frame and is truncated at that point with EOF-ERROR. "
     "The PHY will eventually force ERROR code onto the line. This will limit "
     "the frame transmission run-off in case of Application logic error.  This "
     "value has to be a multiple of the Data path width (in bytes) inside the "
     "MAC , per the  following: 100G: Multiple of 16 40G: Multiple of 8 10G: "
     "Multiple of 4 1G: Multiple of 1."},

};
reg_decoder_t mcmac0_maxtxjabsize_flds = {1, mcmac0_maxtxjabsize_fld_list, 16};

reg_decoder_fld_t mcmac0_maxrxjabsize_fld_list[] = {
    {"maxrxjabsize",
     15,
     0,
     0,
     10803,
     "This field determines the Jabber Size for the incoming (Receive) frames "
     "on this Channel. When the length of the current incoming frame on this "
     "Channel equals or exceeds the value programmed in this field, the Frame "
     "is considered a Jabber Frame and istruncated at that point. The Frame "
     "Status is updated with a Jabber Error and the rest of the Jabbered frame "
     "that is being received is ignored.   This value has to be a multiple of "
     "the Data path width (in bytes) inside the MAC,per the following: 100G: "
     "Multiple of 16 40G: Multiple of 8 10G: Multiple of 4 1G: Multiple of 1."},

};
reg_decoder_t mcmac0_maxrxjabsize_flds = {1, mcmac0_maxrxjabsize_fld_list, 16};

reg_decoder_fld_t mcmac0_txpfcvec_fld_list[] = {
    {"txpfcvec",
     7,
     0,
     0,
     10804,
     "Each bit in this field determines whether the Priority is enabled or "
     "disabled for monitoring PFC transmission. This field is also transmitted "
     "in the Priority Vector field in the PFC Pause Frame.  "},

};
reg_decoder_t mcmac0_txpfcvec_flds = {1, mcmac0_txpfcvec_fld_list, 16};

reg_decoder_fld_t mcmac0_vlantag1_fld_list[] = {
    {"vlantag1",
     15,
     0,
     0,
     10805,
     "This field is used to identify the first VLAN Tag in the incoming frames "
     "on this Channel. This helps in programming the outer VLAN Tag based on "
     "Application requirements."},

};
reg_decoder_t mcmac0_vlantag1_flds = {1, mcmac0_vlantag1_fld_list, 16};

reg_decoder_fld_t mcmac0_vlantag2_fld_list[] = {
    {"vlantag2",
     15,
     0,
     0,
     10806,
     "This field is used to identify the second VLAN Tag in the incoming "
     "frames on this Channel. This helps in programming the inner VLAN Tag "
     "based on Application requirements."},

};
reg_decoder_t mcmac0_vlantag2_flds = {1, mcmac0_vlantag2_fld_list, 16};

reg_decoder_fld_t mcmac0_vlantag3_fld_list[] = {
    {"vlantag3",
     15,
     0,
     0,
     10807,
     "This field is used to identify the third VLAN Tag in the incoming frames "
     "on this Channel. This helps in programming the innermost VLAN Tag based "
     "on Application requirements."},

};
reg_decoder_t mcmac0_vlantag3_flds = {1, mcmac0_vlantag3_fld_list, 16};

reg_decoder_fld_t mcmac0_macaddrlo_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     10808,
     "This byte contains the first Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac0_macaddrlo_flds = {1, mcmac0_macaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac0_macaddrmid_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     10809,
     "This byte contains the third Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac0_macaddrmid_flds = {1, mcmac0_macaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac0_macaddrhi_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     10810,
     "This byte contains the fifth Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac0_macaddrhi_flds = {1, mcmac0_macaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac0_mchasht1_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10811,
     "This field contains the bits[15:0] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac0_mchasht1_flds = {1, mcmac0_mchasht1_fld_list, 16};

reg_decoder_fld_t mcmac0_mchasht2_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10812,
     "This field contains the bits[31:16] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac0_mchasht2_flds = {1, mcmac0_mchasht2_fld_list, 16};

reg_decoder_fld_t mcmac0_mchasht3_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10813,
     "This field contains the bits[47:32] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac0_mchasht3_flds = {1, mcmac0_mchasht3_fld_list, 16};

reg_decoder_fld_t mcmac0_mchasht4_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10814,
     "This field contains the bits[63:48] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac0_mchasht4_flds = {1, mcmac0_mchasht4_fld_list, 16};

reg_decoder_fld_t mcmac0_fcfrmgen_fld_list[] = {
    {"fcfrmgen",
     0,
     0,
     0,
     10815,
     "This is set by Software for the UMAC to transmit the PAUSE Control frame "
     "on this Channel. When this bit is set, the UMAC transmits the PAUSE "
     "control frame by using the values programmed in various PAUSE Control "
     "specific registers (SA/PAUSE_TIME) on this Channel. Once the PAUSE "
     "Control frame is transmitted, the bit is cleared by the UMAC.   The "
     "following are the steps for generating the PAUSE Control frame: 1) Read "
     "and Verify this bit is cleared. If not cleared wait until the bit is "
     "cleared (polling) 2) Program the Pause Frame SA and PAUSE_TIME registers "
     "3) Set this bit. 4) Poll this bit until the bit is cleared."},

};
reg_decoder_t mcmac0_fcfrmgen_flds = {1, mcmac0_fcfrmgen_fld_list, 16};

reg_decoder_fld_t mcmac0_fcdaddrlo_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     10816,
     "This byte contains the first Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac0_fcdaddrlo_flds = {1, mcmac0_fcdaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac0_fcdaddrmid_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     10817,
     "This byte contains the third Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac0_fcdaddrmid_flds = {1, mcmac0_fcdaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac0_fcdaddrhi_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     10818,
     "This byte contains the fifth Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac0_fcdaddrhi_flds = {1, mcmac0_fcdaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac0_fcsaddrlo_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     10819,
     "This byte contains the first Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac0_fcsaddrlo_flds = {1, mcmac0_fcsaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac0_fcsaddrmid_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     10820,
     "This byte contains the third Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac0_fcsaddrmid_flds = {1, mcmac0_fcsaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac0_fcsaddrhi_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     10821,
     "This byte contains the fifth Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac0_fcsaddrhi_flds = {1, mcmac0_fcsaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac0_fcpausetime_fld_list[] = {
    {"fcpausetime",
     15,
     0,
     0,
     10822,
     "This contains the Pause-Time value that is used in the generated PAUSE "
     "Control Frame under software control for this Channel. The 16-bit value "
     "is sent out as defined in the IEEE 802.3 Specification with most "
     "significant byte first followed by least significant byte."},

};
reg_decoder_t mcmac0_fcpausetime_flds = {1, mcmac0_fcpausetime_fld_list, 16};

reg_decoder_fld_t mcmac0_xoffpausetime_fld_list[] = {
    {"xoffpausetime",
     15,
     0,
     0,
     10823,
     "This field contains the Pause-Time value that is used in the generated "
     "PAUSE Control Frame when the XOFF PAUSE Frame is to be transmitted. This "
     "field is used both in the regular PAUSE flow Control Frames and Priority "
     "PAUSE Flow Control Frames. The 16-bit value is sent out as defined in "
     "the IEEE 802.3 Specification with most significant byte first followed "
     "by least significant byte"},

};
reg_decoder_t mcmac0_xoffpausetime_flds = {
    1, mcmac0_xoffpausetime_fld_list, 16};

reg_decoder_fld_t mcmac0_xonpausetime_fld_list[] = {
    {"xonpausetime",
     15,
     0,
     0,
     10824,
     "This field contains the Pause-Time value that is used in the generated "
     "PAUSE Control Frame when the XON PAUSE Frame is to be transmitted. This "
     "field is used both in the regular PAUSE flow Control Frames and Priority "
     "PAUSE Flow Control Frames. The 16-bit value is sent out as defined in "
     "the IEEE 802.3 Specification with most significant byte first followed "
     "by least significant byte"},

};
reg_decoder_t mcmac0_xonpausetime_flds = {1, mcmac0_xonpausetime_fld_list, 16};

reg_decoder_fld_t mcmac0_txtsinfo_fld_list[] = {
    {"tsid",
     1,
     0,
     0,
     10825,
     "This field contains the Transmit Timestamp ID associated with the Time "
     "Stamp that is present in the TimeStamp Value Register."},

    {"tsvld",
     15,
     15,
     0,
     10826,
     "This bit indicates whether the Transmit Timestamp Value Register has "
     "valid Value or not. This bit is set when the Timestamp Value register "
     "has been loaded with a new Timestamp Value from the TimeStamp FIFO."},

};
reg_decoder_t mcmac0_txtsinfo_flds = {2, mcmac0_txtsinfo_fld_list, 16};

reg_decoder_fld_t mcmac0_tsv0_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10827,
     "This field contains the bits [15:0] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac0_tsv0_flds = {1, mcmac0_tsv0_fld_list, 16};

reg_decoder_fld_t mcmac0_tsv1_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10828,
     "This field contains the bits [31:16] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac0_tsv1_flds = {1, mcmac0_tsv1_fld_list, 16};

reg_decoder_fld_t mcmac0_tsv2_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10829,
     "This field contains the bits [47:32] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac0_tsv2_flds = {1, mcmac0_tsv2_fld_list, 16};

reg_decoder_fld_t mcmac0_tsv3_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10830,
     "This field contains the bits [63:48] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac0_tsv3_flds = {1, mcmac0_tsv3_fld_list, 16};

reg_decoder_fld_t mcmac0_txtsdelta_fld_list[] = {
    {"txtsdelta",
     15,
     0,
     0,
     10831,
     "This is the delta value that is subtracted from the latched Tx Timestamp "
     "Value before it is passed along with the Frame SOF."},

};
reg_decoder_t mcmac0_txtsdelta_flds = {1, mcmac0_txtsdelta_fld_list, 16};

reg_decoder_fld_t mcmac0_rxtsdelta_fld_list[] = {
    {"rxtsdelta",
     15,
     0,
     0,
     10832,
     "This is the delta value that is subtracted from the latched Rx Timestamp "
     "Value before it is passed along with the Frame SOF."},

};
reg_decoder_t mcmac0_rxtsdelta_flds = {1, mcmac0_rxtsdelta_fld_list, 16};

reg_decoder_fld_t mcmac0_minframesize_fld_list[] = {
    {"minframesize",
     7,
     0,
     0,
     10833,
     "This field is used to determine the MinFrameSize in Transmit and Receive "
     "operation.  The MinFrameSize should be programmed with a value which is "
     "multiple of 4 and with a minimum value of 32."},

};
reg_decoder_t mcmac0_minframesize_flds = {1, mcmac0_minframesize_fld_list, 16};

reg_decoder_fld_t mcmac0_txvlantag_fld_list[] = {
    {"txvlantag",
     15,
     0,
     0,
     10834,
     "This defines the VLAN Tag that is used for comparing and identifying "
     "VLAN frames on Transmit Side. "},

};
reg_decoder_t mcmac0_txvlantag_flds = {1, mcmac0_txvlantag_fld_list, 16};

reg_decoder_fld_t mcmac0_fint0_fld_list[] = {
    {"ovrdint10", 1, 0, 0, 10835, "TX underrun Interrupt override"},

};
reg_decoder_t mcmac0_fint0_flds = {1, mcmac0_fint0_fld_list, 16};

reg_decoder_fld_t mcmac0_fint1_fld_list[] = {
    {"ovrdint32", 1, 0, 0, 10836, "TX timestamp fifo overflow override"},

};
reg_decoder_t mcmac0_fint1_flds = {1, mcmac0_fint1_fld_list, 16};

reg_decoder_fld_t mcmac0_fint2_fld_list[] = {
    {"ovrdint654", 2, 0, 0, 10837, "RX Local Fault Interrupt override"},

};
reg_decoder_t mcmac0_fint2_flds = {1, mcmac0_fint2_fld_list, 16};

reg_decoder_fld_t mcmac0_slotclkcnt_fld_list[] = {
    {"slotclkcnt",
     7,
     0,
     0,
     10838,
     "When this field is set to zero, the transmit and receive MAC uses "
     "internal DataValid/DataReady to count the valid clocks for the Slot "
     "Timer Logic. In this mode the Slot is 512 Bit times based on the line "
     "rate.  When this field is non-zero, this field determines the number of "
     "Clocks (Core_clk_i) for the Slot Timer.   The Slot Timer is used to "
     "decrement the PAUSE Quanta."},

};
reg_decoder_t mcmac0_slotclkcnt_flds = {1, mcmac0_slotclkcnt_fld_list, 16};

reg_decoder_fld_t mcmac0_txdebug_fld_list[] = {
    {"stompeoperr",
     0,
     0,
     0,
     10839,
     "When set the EOP/ERR is ignored by the MAC."},

    {"txlfault",
     1,
     1,
     0,
     10840,
     "When set, the MAC continuously transmits L_FAULT ordered set."},

    {"txrfault",
     2,
     2,
     0,
     10841,
     "When set, the MAC continuously transmits R_FAULT ordered set."},

    {"txidle",
     3,
     3,
     0,
     10842,
     "When set, the MAC continuously transmits IDLE Pattern."},

    {"txtestpattern",
     4,
     4,
     0,
     10843,
     "This should be set when the PCS is programmed to transmit Test Pattern. "
     "This bit does not have any functionality in the MAC other than in the "
     "generation of TX_GOOD signal output. "},

    {"forcerxxoff",
     5,
     5,
     0,
     10844,
     "When set, the XOFF (for all priorities) is forced on APP_FIFO Interface "
     "when a regular PAUSE frame is received and the Transmit MAC is not "
     "disabled. When clear, normal PAUSE frame reception behavior of disabling "
     "TxMAC"},

    {"txautodraintxdisable",
     6,
     6,
     0,
     10845,
     "TxAutoDrainonTxDisable: When set, the TxAutoDrain function is enabled "
     "when the TXMAC is disabled (TXEnable is set to 1'b0). When clear, the "
     "frames are not drained when the MAC is disabled."},

    {"macdebug0",
     7,
     7,
     0,
     10846,
     "MAC Debug #0. This should be treated as reserved and not modified by "
     "User."},

    {"txvlantagena",
     8,
     8,
     0,
     10847,
     "This enables the VLAN Tag comparison and identification of VLAN frames "
     "on Transmit Side"},

    {"ovrdtxdisonpause",
     9,
     9,
     0,
     10848,
     "When set, the TxDisable on RxPause function is overridden. The Tx Is not "
     "disabled, but the signal ff_Rxfcxoff signal is generated on the APP_FIFO "
     "Interface. When cleared, the normal Pause function is enabled where the "
     "TXMAC is disabled for the duration of PAUSE Time."},

};
reg_decoder_t mcmac0_txdebug_flds = {10, mcmac0_txdebug_fld_list, 16};

reg_decoder_fld_t mcmac0_spare0_fld_list[] = {
    {"spare0", 7, 0, 0, 10849, "Spare0"},

};
reg_decoder_t mcmac0_spare0_flds = {1, mcmac0_spare0_fld_list, 16};

reg_decoder_fld_t mcmac0_useccntr_fld_list[] = {
    {"useccntr",
     15,
     0,
     0,
     10850,
     "This determines the number of clocks (Core_clk_i) for a Microsecond. "
     "These value is used in determining the number of microseconds the MAC is "
     "in PAUSE (either regular pause or Priority PAUSE). This is used both by "
     "transmit and receive MAC."},

};
reg_decoder_t mcmac0_useccntr_flds = {1, mcmac0_useccntr_fld_list, 16};

reg_decoder_fld_t mcmac1_ctrl_fld_list[] = {
    {"txenable",
     0,
     0,
     0,
     10851,
     "When set, this Channel's MAC transmitter is enabled and it will transmit "
     "frames from the Transmit FIFO/Application logic on to the PCS Interface. "
     "When reset, this Channel's MAC's transmitter is disabled and will not "
     "transmit any frames. "},

    {"rxenable",
     1,
     1,
     0,
     10852,
     "When set, this Channel's MAC receiver is enabled and it will receive "
     "frames from the PCS and transfer them to Receive FIFO/Application logic. "
     "When reset, this Channel's MAC Core receiver is disabled and will not "
     "receive any frames."},

    {"swreset",
     2,
     2,
     0,
     10853,
     "Setting this bit resets this Channel's various MAC logic to the default "
     "state. The contents of the Configuration Registers related to this "
     "Channel are not affected by this bit.  This Channel's MAC logic is "
     "continued to be in reset state while this bit is set."},

    {"maclpbk",
     3,
     3,
     0,
     10854,
     "Setting this bit enables the Loopback on the MAC-PCS Interface for this "
     "Channel. The Transmit data and controls are looped back on to the "
     "receive MAC module for this Channel.  When this loopback is enabled, the "
     "data received from the PCS for this channel is ignored."},

    {"faultovrd",
     4,
     4,
     0,
     10855,
     "When set, overrides the Ch#0 Link Fault Status. For diagnostics only"},

    {"txdrn",
     5,
     5,
     0,
     10856,
     "Setting this bit causes transmit path to enter into Drain mode where all "
     "the data from the TXFIFO is drained out."},

};
reg_decoder_t mcmac1_ctrl_flds = {6, mcmac1_ctrl_fld_list, 16};

reg_decoder_fld_t mcmac1_txconfig_fld_list[] = {
    {"disfcs",
     0,
     0,
     0,
     10857,
     "When set, the FCS calculation and insertion logic is disabled in the "
     "transmit path for this Channel. FCS insertion is disabled for all the "
     "frames. When reset, the FCS is calculated on all frames and insertion is "
     "based on the per frame control signal (ff_txchdisfcs for this Channel).  "
     "When the Disable FCS Insertion is set, it is expected that the frames "
     "coming in from the Application logic will meet the 64-byte MinFrameSize "
     "requirements and contains the FCS field to make the outgoing frame IEEE "
     "802.3 specification Compliant."},

    {"invfcs",
     1,
     1,
     0,
     10858,
     "When set, the UMAC inverts the FCS field that is being inserted into the "
     "outgoing frames for this Channel. When reset, the UMAC Core operates "
     "normally for FCS insertion for this Channel.   Note: According to the "
     "IEEE 802.3 specification, the FCS is calculated beginning from the first "
     "byte of DA until the last byte of the DATA/PAD field. The calculated FCS "
     "is then inverted and sent out MSB first. So in normal mode, the "
     "calculated FCS is inverted before being inserted into the outgoing "
     "frame. When the Invert FCS is set, the FCS field is double inverted (No "
     "inversion happens)."},

    {"ifglength",
     7,
     2,
     0,
     10859,
     "This determines the minimum IFG (Inter Frame Gap) in Bytes, to be "
     "inserted between outgoing frames for this Channel, when the Application "
     "logic has back to back frames, and no Frame Pacing is defined. The IEEE "
     "802.3 specification specifies a minimum IFG of 96 bit-times between "
     "frames. The following are the permissible values for the IFG Length "
     "field.   For 10M, 100M, and 1000M, the range is as follows. Minimum: 1 "
     "Maximum: 63.  For 10G the range is as follows: Minimum: 8 Maximum: 63.  "
     "For 40G, 100G the range is as follows: Minimum: 12 Maximum: 63.  In case "
     "of 10G speed and above, the Deficit IDLE Counter is implemented to "
     "maintain the average rate of the programmed IFG.  The "
     "ff_txnextfrifg[7:0] value temporarily overrides the IFG programmed in "
     "this register.  When the IFG Length field is programmed with Zero, then "
     "the UMAC operates in Zero length IFG mode where the frames are "
     "transmitted back to back while horning the START alignment to Lane#0."},

    {"fcfrmgen",
     8,
     8,
     0,
     10860,
     "When set, this Channel is enabled for transmission of PAUSE Control "
     "Frames. The PAUSE Control frames are transmitted either on Software "
     "control or based on Application logic controls. The parameters for the "
     "generated PAUSE Frame are configured into the registers. When reset, "
     "this Channel does not generate a PAUSE Control Frame."},

    {"pfcfrmgen",
     9,
     9,
     0,
     10861,
     "When set, this Channel is enabled for transmission of PFC Control "
     "Frames. The PFC Control frames are transmitted based on Application "
     "logic's controls. When reset, this Channel doesn't generate priority "
     "PAUSE Control Frame."},

    {"prelength",
     13,
     11,
     0,
     10862,
     "This defines length of the Preamble that is used for every outgoing "
     "frame for this Channel.  Based on the MAC's operating speed as specified "
     "below:  10G Rates and above - 3'b000: Default 8-bytes (START, 6 bytes of "
     "PRE, 1 byte of SFD) - 3'b001: Reserved - 3'b010: Reserved - 3'b011: "
     "Reserved - 3'b100: bytes (START, 2 bytes of PRE, 1 byte of SFD). - "
     "3'b101: Reserved - 3'b111: Reserved - 3'b111: Reserved  1G Rates and "
     "below - 3'b000: Default 8-bytes (7 Bytes of PRE plus 1 Byte of SFD) - "
     "3'b001: 1 Byte of SFD only. - 3'b010: 1 Byte of PRE plus 1 Byte of SFD - "
     "3'b011: 2 Bytes of PRE plus 1 Byte of SFD - 3'b100: 3 Bytes of PRE plus "
     "1 Byte of SFD - 3'b101: 4 Bytes of PRE plus 1 Byte of SFD - 3'b110: 5 "
     "Bytes of PRE plus 1 Byte of SFD - 3'b111: 6 Bytes of PRE plus 1 Byte of "
     "SFD"},

    {"enfrmpace",
     14,
     14,
     0,
     10863,
     "Enable Pacing of Frames using the ff_txnextfrmifg value. When this is "
     "set, the value from ff_txnextfrmifg is used as the IFG between frames. "
     "When this is not set, the value programmed in IFG Length is used for all "
     "frames."},

    {"enautodrnonflt",
     15,
     15,
     0,
     10864,
     "Enables Auto Draining of Frames from FIFO Interface when the MAC "
     "receiver is in Fault state."},

};
reg_decoder_t mcmac1_txconfig_flds = {8, mcmac1_txconfig_fld_list, 16};

reg_decoder_fld_t mcmac1_rxconfig_fld_list[] = {
    {"disfcschk",
     0,
     0,
     0,
     10865,
     "When set, the UMAC doesn't perform FCS Checking on the incoming frames "
     "for this Channel and the CRC Error status is not set for any of the "
     "frames. When reset, the UMAC operates normally and checks FCS on every "
     "incoming frame, for this Channel. Frames with CRC Errors are reported in "
     "the Frame Status."},

    {"stripfcs",
     1,
     1,
     0,
     10866,
     "When set, the FCS field is stripped of the frame before the Frame is "
     "transferred to the Application logic for this Channel's frames. The "
     "Frame Length field is updated to reflect the new length (without the FCS "
     "field). When reset, the FCS field is not stripped and is part of the "
     "frame that is transferred to Application logic.   Irrespective of the "
     "Strip FCS field, FCS Checking is performed on every frame unless Disable "
     "FCS Check is set. When theFCS field is checked, the CRC Error status is "
     "reported for every frame. "},

    {"vlanchk",
     3,
     2,
     0,
     10867,
     "Per the IEEE 802.3 specification, the minFrameSize and maxFrameSize "
     "values for untagged frames are set to 64 bytes and 1518 bytes "
     "respectively. The values for tagged (VLAN Tagged) frame sizes are "
     "updated to account for 4-bytes of VLAN Tag and the maxFrameSize value is "
     "updated to 1522 Bytes.   The UMAC supports up to 3 VLAN Tags per frame. "
     "It can adjust the maxFrameSize field to support up to 3 VLAN Tags (12 "
     "Bytes), based on the number of VLAN tags that are present in the "
     "incoming frame. This helps in false MaxFrameSize violation reporting for "
     "frames with VLAN tags that exceed 1518 bytes but are under the "
     "1518+tagbytes in length.  The UMAC will use the value programmed in this "
     "field to determine the number of VLAN's for which the MAC will check "
     "(for this Channel) and update the MaxFrameSize field accordingly.  "
     "2'b00: Disable VLAN Checking. 2'b01: Check for up to 1 VLAN Tag (Use the "
     "VLAN Tag#1 Register) 2'b10: Check for up to 2 VLAN Tags (Use VLAN Tag#1 "
     "and #2 Registers) 2'b11: Check for up to 3 VLAN Tags (Use VLAN Tag#1, "
     "#2, and #3Registers)"},

    {"promiscuous",
     4,
     4,
     0,
     10868,
     "When set, the UMAC disables address checking on all frames that are "
     "received on this Channel, and all the frames received on this channel "
     "are passed onto the FIFO Interface. The Frame Status reflects the status "
     "of the Address Checking.  When the bit is cleared, the DA field is "
     "compared against the contents of the MAC Address Register (for Unicast "
     "Frames) and the Multicast Hash table (for Multicast Frames). Frames that "
     "fail topass the Address filtering will be dropped and not transferred to "
     "the FIFO Interface  "},

    {"enrxfcdec",
     5,
     5,
     0,
     10869,
     "When set, the UMAC Core is enabled for Flow-Control decode operation for "
     "this Channel and it will decode all the incoming frames for PAUSE "
     "Control Frames as specified in the IEEE 802.3 Specification.   If the "
     "UMAC Core receives a valid PAUSE Control Frame, it will disable the "
     "transmission of user data frames for the time given in the PAUSE_TIME "
     "field of the PAUSE Control Frame.  If the UMAC Core receives a valid "
     "Priority PAUSE Control Frame, it will load the timers and providethe "
     "XOFF indication to the Application logic (ff_rxch0pfcxoff[7:0]) based on "
     "the Time Vector fields in the 8 priorities.  When reset, the "
     "Flow-Control Operation in the UMAC is disabled for this Channel, and it "
     "does not decode the frames for PAUSE Control Frames or Priority Pause "
     "Flow Control Frames."},

    {"prelength",
     7,
     7,
     0,
     10870,
     "This defines the START/PREAMBLE/SFD length that is expected of all the "
     "frames received on this Channel.  1'b0: Default 8-bytes (START, 6 bytes "
     "of PREAMBLE, 1 byte of SFD) 1'b1: 4-bytes (START, 2 bytes of PREAMBLE, 1 "
     "byte of SFD).  This is applicable when the Channel is operating at 10G "
     "rates and above."},

    {"filterpf",
     8,
     8,
     0,
     10871,
     "When this bit is set, the UMAC will filter out the frames with DA "
     "matching the reserved Multicast PAUSE Control Frame address "
     "(01:80:C2:00:00:01) from being sent to the Application Logic.   When "
     "this bit is reset, the UMAC does not filter out the frames with the DA "
     "matching the reserved Multicast PAUSE Control Frame address "
     "(01:80:C2:00:00:01).  This bit has no effect on actual "
     "processing/decoding on the PAUSE (Priority PAUSE) Control frames, which "
     "is controlled thru theEnable Rx Flow Control Decode bit.  When the PAUSE "
     "control frames are passed to the application logic, (Filter PAUSE "
     "Frames) is not set, but the application logic can still identify the "
     "PAUSE Control frames using Bit[31] in ff_rxsts that is sent out with the "
     "frame."},

    {"enearlyeofdet",
     9,
     9,
     0,
     10872,
     "This should be set only when the UMAC is configured for 4x10G mode and "
     "the Low Latency option is selected in the PCS. This enables the "
     "detection of Early EOF indication from PCS which helps in reducing the "
     "latency.  Note: This is to be set only when the UMAC is operating in "
     "4x10G Mode. Setting this in other modes will cause the UMAC to have "
     "unexpected behavior."},

};
reg_decoder_t mcmac1_rxconfig_flds = {8, mcmac1_rxconfig_fld_list, 16};

reg_decoder_fld_t mcmac1_maxfrmsize_fld_list[] = {
    {"maxfrmsize",
     15,
     0,
     0,
     10873,
     "This field determines the Maximum Frame Size for untagged frames that is "
     "used in checking the MaxFrameLength violations on this Channel. For "
     "frames larger than this value, the MaxFrameLength Error bit is set in "
     "the Frame Status."},

};
reg_decoder_t mcmac1_maxfrmsize_flds = {1, mcmac1_maxfrmsize_fld_list, 16};

reg_decoder_fld_t mcmac1_maxtxjabsize_fld_list[] = {
    {"maxtxjabsize",
     15,
     0,
     0,
     10874,
     "This field determines the Jabber Size for the outgoing (Transmit) frames "
     "on this Channel. When the length of the current outgoing frame on this "
     "Channel exceeds the value programmed in this field, the Frame is "
     "considered a Jabber Frame and is truncated at that point with EOF-ERROR. "
     "The PHY will eventually force ERROR code onto the line. This will limit "
     "the frame transmission run-off in case of Application logic error.  This "
     "value has to be a multiple of the Data path width (in bytes) inside the "
     "MAC , per the  following: 100G: Multiple of 16 40G: Multiple of 8 10G: "
     "Multiple of 4 1G: Multiple of 1."},

};
reg_decoder_t mcmac1_maxtxjabsize_flds = {1, mcmac1_maxtxjabsize_fld_list, 16};

reg_decoder_fld_t mcmac1_maxrxjabsize_fld_list[] = {
    {"maxrxjabsize",
     15,
     0,
     0,
     10875,
     "This field determines the Jabber Size for the incoming (Receive) frames "
     "on this Channel. When the length of the current incoming frame on this "
     "Channel equals or exceeds the value programmed in this field, the Frame "
     "is considered a Jabber Frame and istruncated at that point. The Frame "
     "Status is updated with a Jabber Error and the rest of the Jabbered frame "
     "that is being received is ignored.   This value has to be a multiple of "
     "the Data path width (in bytes) inside the MAC,per the following: 100G: "
     "Multiple of 16 40G: Multiple of 8 10G: Multiple of 4 1G: Multiple of 1."},

};
reg_decoder_t mcmac1_maxrxjabsize_flds = {1, mcmac1_maxrxjabsize_fld_list, 16};

reg_decoder_fld_t mcmac1_txpfcvec_fld_list[] = {
    {"txpfcvec",
     7,
     0,
     0,
     10876,
     "Each bit in this field determines whether the Priority is enabled or "
     "disabled for monitoring PFC transmission. This field is also transmitted "
     "in the Priority Vector field in the PFC Pause Frame.  "},

};
reg_decoder_t mcmac1_txpfcvec_flds = {1, mcmac1_txpfcvec_fld_list, 16};

reg_decoder_fld_t mcmac1_vlantag1_fld_list[] = {
    {"vlantag1",
     15,
     0,
     0,
     10877,
     "This field is used to identify the first VLAN Tag in the incoming frames "
     "on this Channel. This helps in programming the outer VLAN Tag based on "
     "Application requirements."},

};
reg_decoder_t mcmac1_vlantag1_flds = {1, mcmac1_vlantag1_fld_list, 16};

reg_decoder_fld_t mcmac1_vlantag2_fld_list[] = {
    {"vlantag2",
     15,
     0,
     0,
     10878,
     "This field is used to identify the second VLAN Tag in the incoming "
     "frames on this Channel. This helps in programming the inner VLAN Tag "
     "based on Application requirements."},

};
reg_decoder_t mcmac1_vlantag2_flds = {1, mcmac1_vlantag2_fld_list, 16};

reg_decoder_fld_t mcmac1_vlantag3_fld_list[] = {
    {"vlantag3",
     15,
     0,
     0,
     10879,
     "This field is used to identify the third VLAN Tag in the incoming frames "
     "on this Channel. This helps in programming the innermost VLAN Tag based "
     "on Application requirements."},

};
reg_decoder_t mcmac1_vlantag3_flds = {1, mcmac1_vlantag3_fld_list, 16};

reg_decoder_fld_t mcmac1_macaddrlo_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     10880,
     "This byte contains the first Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac1_macaddrlo_flds = {1, mcmac1_macaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac1_macaddrmid_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     10881,
     "This byte contains the third Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac1_macaddrmid_flds = {1, mcmac1_macaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac1_macaddrhi_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     10882,
     "This byte contains the fifth Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac1_macaddrhi_flds = {1, mcmac1_macaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac1_mchasht1_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10883,
     "This field contains the bits[15:0] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac1_mchasht1_flds = {1, mcmac1_mchasht1_fld_list, 16};

reg_decoder_fld_t mcmac1_mchasht2_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10884,
     "This field contains the bits[31:16] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac1_mchasht2_flds = {1, mcmac1_mchasht2_fld_list, 16};

reg_decoder_fld_t mcmac1_mchasht3_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10885,
     "This field contains the bits[47:32] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac1_mchasht3_flds = {1, mcmac1_mchasht3_fld_list, 16};

reg_decoder_fld_t mcmac1_mchasht4_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10886,
     "This field contains the bits[63:48] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac1_mchasht4_flds = {1, mcmac1_mchasht4_fld_list, 16};

reg_decoder_fld_t mcmac1_fcfrmgen_fld_list[] = {
    {"fcfrmgen",
     0,
     0,
     0,
     10887,
     "This is set by Software for the UMAC to transmit the PAUSE Control frame "
     "on this Channel. When this bit is set, the UMAC transmits the PAUSE "
     "control frame by using the values programmed in various PAUSE Control "
     "specific registers (SA/PAUSE_TIME) on this Channel. Once the PAUSE "
     "Control frame is transmitted, the bit is cleared by the UMAC.   The "
     "following are the steps for generating the PAUSE Control frame: 1) Read "
     "and Verify this bit is cleared. If not cleared wait until the bit is "
     "cleared (polling) 2) Program the Pause Frame SA and PAUSE_TIME registers "
     "3) Set this bit. 4) Poll this bit until the bit is cleared."},

};
reg_decoder_t mcmac1_fcfrmgen_flds = {1, mcmac1_fcfrmgen_fld_list, 16};

reg_decoder_fld_t mcmac1_fcdaddrlo_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     10888,
     "This byte contains the first Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac1_fcdaddrlo_flds = {1, mcmac1_fcdaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac1_fcdaddrmid_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     10889,
     "This byte contains the third Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac1_fcdaddrmid_flds = {1, mcmac1_fcdaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac1_fcdaddrhi_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     10890,
     "This byte contains the fifth Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac1_fcdaddrhi_flds = {1, mcmac1_fcdaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac1_fcsaddrlo_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     10891,
     "This byte contains the first Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac1_fcsaddrlo_flds = {1, mcmac1_fcsaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac1_fcsaddrmid_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     10892,
     "This byte contains the third Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac1_fcsaddrmid_flds = {1, mcmac1_fcsaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac1_fcsaddrhi_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     10893,
     "This byte contains the fifth Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac1_fcsaddrhi_flds = {1, mcmac1_fcsaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac1_fcpausetime_fld_list[] = {
    {"fcpausetime",
     15,
     0,
     0,
     10894,
     "This contains the Pause-Time value that is used in the generated PAUSE "
     "Control Frame under software control for this Channel. The 16-bit value "
     "is sent out as defined in the IEEE 802.3 Specification with most "
     "significant byte first followed by least significant byte."},

};
reg_decoder_t mcmac1_fcpausetime_flds = {1, mcmac1_fcpausetime_fld_list, 16};

reg_decoder_fld_t mcmac1_xoffpausetime_fld_list[] = {
    {"xoffpausetime",
     15,
     0,
     0,
     10895,
     "This field contains the Pause-Time value that is used in the generated "
     "PAUSE Control Frame when the XOFF PAUSE Frame is to be transmitted. This "
     "field is used both in the regular PAUSE flow Control Frames and Priority "
     "PAUSE Flow Control Frames. The 16-bit value is sent out as defined in "
     "the IEEE 802.3 Specification with most significant byte first followed "
     "by least significant byte"},

};
reg_decoder_t mcmac1_xoffpausetime_flds = {
    1, mcmac1_xoffpausetime_fld_list, 16};

reg_decoder_fld_t mcmac1_xonpausetime_fld_list[] = {
    {"xonpausetime",
     15,
     0,
     0,
     10896,
     "This field contains the Pause-Time value that is used in the generated "
     "PAUSE Control Frame when the XON PAUSE Frame is to be transmitted. This "
     "field is used both in the regular PAUSE flow Control Frames and Priority "
     "PAUSE Flow Control Frames. The 16-bit value is sent out as defined in "
     "the IEEE 802.3 Specification with most significant byte first followed "
     "by least significant byte"},

};
reg_decoder_t mcmac1_xonpausetime_flds = {1, mcmac1_xonpausetime_fld_list, 16};

reg_decoder_fld_t mcmac1_txtsinfo_fld_list[] = {
    {"tsid",
     1,
     0,
     0,
     10897,
     "This field contains the Transmit Timestamp ID associated with the Time "
     "Stamp that is present in the TimeStamp Value Register."},

    {"tsvld",
     15,
     15,
     0,
     10898,
     "This bit indicates whether the Transmit Timestamp Value Register has "
     "valid Value or not. This bit is set when the Timestamp Value register "
     "has been loaded with a new Timestamp Value from the TimeStamp FIFO."},

};
reg_decoder_t mcmac1_txtsinfo_flds = {2, mcmac1_txtsinfo_fld_list, 16};

reg_decoder_fld_t mcmac1_tsv0_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10899,
     "This field contains the bits [15:0] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac1_tsv0_flds = {1, mcmac1_tsv0_fld_list, 16};

reg_decoder_fld_t mcmac1_tsv1_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10900,
     "This field contains the bits [31:16] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac1_tsv1_flds = {1, mcmac1_tsv1_fld_list, 16};

reg_decoder_fld_t mcmac1_tsv2_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10901,
     "This field contains the bits [47:32] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac1_tsv2_flds = {1, mcmac1_tsv2_fld_list, 16};

reg_decoder_fld_t mcmac1_tsv3_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10902,
     "This field contains the bits [63:48] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac1_tsv3_flds = {1, mcmac1_tsv3_fld_list, 16};

reg_decoder_fld_t mcmac1_txtsdelta_fld_list[] = {
    {"txtsdelta",
     15,
     0,
     0,
     10903,
     "This is the delta value that is subtracted from the latched Tx Timestamp "
     "Value before it is passed along with the Frame SOF."},

};
reg_decoder_t mcmac1_txtsdelta_flds = {1, mcmac1_txtsdelta_fld_list, 16};

reg_decoder_fld_t mcmac1_rxtsdelta_fld_list[] = {
    {"rxtsdelta",
     15,
     0,
     0,
     10904,
     "This is the delta value that is subtracted from the latched Rx Timestamp "
     "Value before it is passed along with the Frame SOF."},

};
reg_decoder_t mcmac1_rxtsdelta_flds = {1, mcmac1_rxtsdelta_fld_list, 16};

reg_decoder_fld_t mcmac1_minframesize_fld_list[] = {
    {"minframesize",
     7,
     0,
     0,
     10905,
     "This field is used to determine the MinFrameSize in Transmit and Receive "
     "operation.  The MinFrameSize should be programmed with a value which is "
     "multiple of 4 and with a minimum value of 32."},

};
reg_decoder_t mcmac1_minframesize_flds = {1, mcmac1_minframesize_fld_list, 16};

reg_decoder_fld_t mcmac1_txvlantag_fld_list[] = {
    {"txvlantag",
     15,
     0,
     0,
     10906,
     "This defines the VLAN Tag that is used for comparing and identifying "
     "VLAN frames on Transmit Side. "},

};
reg_decoder_t mcmac1_txvlantag_flds = {1, mcmac1_txvlantag_fld_list, 16};

reg_decoder_fld_t mcmac1_fint0_fld_list[] = {
    {"ovrdint10", 1, 0, 0, 10907, "TX underrun Interrupt override"},

};
reg_decoder_t mcmac1_fint0_flds = {1, mcmac1_fint0_fld_list, 16};

reg_decoder_fld_t mcmac1_fint1_fld_list[] = {
    {"ovrdint32", 1, 0, 0, 10908, "TX timestamp fifo overflow override"},

};
reg_decoder_t mcmac1_fint1_flds = {1, mcmac1_fint1_fld_list, 16};

reg_decoder_fld_t mcmac1_fint2_fld_list[] = {
    {"ovrdint654", 2, 0, 0, 10909, "RX Local Fault Interrupt override"},

};
reg_decoder_t mcmac1_fint2_flds = {1, mcmac1_fint2_fld_list, 16};

reg_decoder_fld_t mcmac1_slotclkcnt_fld_list[] = {
    {"slotclkcnt",
     7,
     0,
     0,
     10910,
     "When this field is set to zero, the transmit and receive MAC uses "
     "internal DataValid/DataReady to count the valid clocks for the Slot "
     "Timer Logic. In this mode the Slot is 512 Bit times based on the line "
     "rate.  When this field is non-zero, this field determines the number of "
     "Clocks (Core_clk_i) for the Slot Timer.   The Slot Timer is used to "
     "decrement the PAUSE Quanta."},

};
reg_decoder_t mcmac1_slotclkcnt_flds = {1, mcmac1_slotclkcnt_fld_list, 16};

reg_decoder_fld_t mcmac1_txdebug_fld_list[] = {
    {"stompeoperr",
     0,
     0,
     0,
     10911,
     "When set the EOP/ERR is ignored by the MAC."},

    {"txlfault",
     1,
     1,
     0,
     10912,
     "When set, the MAC continuously transmits L_FAULT ordered set."},

    {"txrfault",
     2,
     2,
     0,
     10913,
     "When set, the MAC continuously transmits R_FAULT ordered set."},

    {"txidle",
     3,
     3,
     0,
     10914,
     "When set, the MAC continuously transmits IDLE Pattern."},

    {"txtestpattern",
     4,
     4,
     0,
     10915,
     "This should be set when the PCS is programmed to transmit Test Pattern. "
     "This bit does not have any functionality in the MAC other than in the "
     "generation of TX_GOOD signal output. "},

    {"forcerxxoff",
     5,
     5,
     0,
     10916,
     "When set, the XOFF (for all priorities) is forced on APP_FIFO Interface "
     "when a regular PAUSE frame is received and the Transmit MAC is not "
     "disabled. When clear, normal PAUSE frame reception behavior of disabling "
     "TxMAC"},

    {"txautodraintxdisable",
     6,
     6,
     0,
     10917,
     "TxAutoDrainonTxDisable: When set, the TxAutoDrain function is enabled "
     "when the TXMAC is disabled (TXEnable is set to 1'b0). When clear, the "
     "frames are not drained when the MAC is disabled."},

    {"macdebug0",
     7,
     7,
     0,
     10918,
     "MAC Debug #0. This should be treated as reserved and not modified by "
     "User."},

    {"txvlantagena",
     8,
     8,
     0,
     10919,
     "This enables the VLAN Tag comparison and identification of VLAN frames "
     "on Transmit Side"},

    {"ovrdtxdisonpause",
     9,
     9,
     0,
     10920,
     "When set, the TxDisable on RxPause function is overridden. The Tx Is not "
     "disabled, but the signal ff_Rxfcxoff signal is generated on the APP_FIFO "
     "Interface. When cleared, the normal Pause function is enabled where the "
     "TXMAC is disabled for the duration of PAUSE Time."},

};
reg_decoder_t mcmac1_txdebug_flds = {10, mcmac1_txdebug_fld_list, 16};

reg_decoder_fld_t mcmac2_ctrl_fld_list[] = {
    {"txenable",
     0,
     0,
     0,
     10921,
     "When set, this Channel's MAC transmitter is enabled and it will transmit "
     "frames from the Transmit FIFO/Application logic on to the PCS Interface. "
     "When reset, this Channel's MAC's transmitter is disabled and will not "
     "transmit any frames. "},

    {"rxenable",
     1,
     1,
     0,
     10922,
     "When set, this Channel's MAC receiver is enabled and it will receive "
     "frames from the PCS and transfer them to Receive FIFO/Application logic. "
     "When reset, this Channel's MAC Core receiver is disabled and will not "
     "receive any frames."},

    {"swreset",
     2,
     2,
     0,
     10923,
     "Setting this bit resets this Channel's various MAC logic to the default "
     "state. The contents of the Configuration Registers related to this "
     "Channel are not affected by this bit.  This Channel's MAC logic is "
     "continued to be in reset state while this bit is set."},

    {"maclpbk",
     3,
     3,
     0,
     10924,
     "Setting this bit enables the Loopback on the MAC-PCS Interface for this "
     "Channel. The Transmit data and controls are looped back on to the "
     "receive MAC module for this Channel.  When this loopback is enabled, the "
     "data received from the PCS for this channel is ignored."},

    {"faultovrd",
     4,
     4,
     0,
     10925,
     "When set, overrides the Ch#0 Link Fault Status. For diagnostics only"},

    {"txdrn",
     5,
     5,
     0,
     10926,
     "Setting this bit causes transmit path to enter into Drain mode where all "
     "the data from the TXFIFO is drained out."},

};
reg_decoder_t mcmac2_ctrl_flds = {6, mcmac2_ctrl_fld_list, 16};

reg_decoder_fld_t mcmac2_txconfig_fld_list[] = {
    {"disfcs",
     0,
     0,
     0,
     10927,
     "When set, the FCS calculation and insertion logic is disabled in the "
     "transmit path for this Channel. FCS insertion is disabled for all the "
     "frames. When reset, the FCS is calculated on all frames and insertion is "
     "based on the per frame control signal (ff_txchdisfcs for this Channel).  "
     "When the Disable FCS Insertion is set, it is expected that the frames "
     "coming in from the Application logic will meet the 64-byte MinFrameSize "
     "requirements and contains the FCS field to make the outgoing frame IEEE "
     "802.3 specification Compliant."},

    {"invfcs",
     1,
     1,
     0,
     10928,
     "When set, the UMAC inverts the FCS field that is being inserted into the "
     "outgoing frames for this Channel. When reset, the UMAC Core operates "
     "normally for FCS insertion for this Channel.   Note: According to the "
     "IEEE 802.3 specification, the FCS is calculated beginning from the first "
     "byte of DA until the last byte of the DATA/PAD field. The calculated FCS "
     "is then inverted and sent out MSB first. So in normal mode, the "
     "calculated FCS is inverted before being inserted into the outgoing "
     "frame. When the Invert FCS is set, the FCS field is double inverted (No "
     "inversion happens)."},

    {"ifglength",
     7,
     2,
     0,
     10929,
     "This determines the minimum IFG (Inter Frame Gap) in Bytes, to be "
     "inserted between outgoing frames for this Channel, when the Application "
     "logic has back to back frames, and no Frame Pacing is defined. The IEEE "
     "802.3 specification specifies a minimum IFG of 96 bit-times between "
     "frames. The following are the permissible values for the IFG Length "
     "field.   For 10M, 100M, and 1000M, the range is as follows. Minimum: 1 "
     "Maximum: 63.  For 10G the range is as follows: Minimum: 8 Maximum: 63.  "
     "For 40G, 100G the range is as follows: Minimum: 12 Maximum: 63.  In case "
     "of 10G speed and above, the Deficit IDLE Counter is implemented to "
     "maintain the average rate of the programmed IFG.  The "
     "ff_txnextfrifg[7:0] value temporarily overrides the IFG programmed in "
     "this register.  When the IFG Length field is programmed with Zero, then "
     "the UMAC operates in Zero length IFG mode where the frames are "
     "transmitted back to back while horning the START alignment to Lane#0."},

    {"fcfrmgen",
     8,
     8,
     0,
     10930,
     "When set, this Channel is enabled for transmission of PAUSE Control "
     "Frames. The PAUSE Control frames are transmitted either on Software "
     "control or based on Application logic controls. The parameters for the "
     "generated PAUSE Frame are configured into the registers. When reset, "
     "this Channel does not generate a PAUSE Control Frame."},

    {"pfcfrmgen",
     9,
     9,
     0,
     10931,
     "When set, this Channel is enabled for transmission of PFC Control "
     "Frames. The PFC Control frames are transmitted based on Application "
     "logic's controls. When reset, this Channel doesn't generate priority "
     "PAUSE Control Frame."},

    {"prelength",
     13,
     11,
     0,
     10932,
     "This defines length of the Preamble that is used for every outgoing "
     "frame for this Channel.  Based on the MAC's operating speed as specified "
     "below:  10G Rates and above - 3'b000: Default 8-bytes (START, 6 bytes of "
     "PRE, 1 byte of SFD) - 3'b001: Reserved - 3'b010: Reserved - 3'b011: "
     "Reserved - 3'b100: bytes (START, 2 bytes of PRE, 1 byte of SFD). - "
     "3'b101: Reserved - 3'b111: Reserved - 3'b111: Reserved  1G Rates and "
     "below - 3'b000: Default 8-bytes (7 Bytes of PRE plus 1 Byte of SFD) - "
     "3'b001: 1 Byte of SFD only. - 3'b010: 1 Byte of PRE plus 1 Byte of SFD - "
     "3'b011: 2 Bytes of PRE plus 1 Byte of SFD - 3'b100: 3 Bytes of PRE plus "
     "1 Byte of SFD - 3'b101: 4 Bytes of PRE plus 1 Byte of SFD - 3'b110: 5 "
     "Bytes of PRE plus 1 Byte of SFD - 3'b111: 6 Bytes of PRE plus 1 Byte of "
     "SFD"},

    {"enfrmpace",
     14,
     14,
     0,
     10933,
     "Enable Pacing of Frames using the ff_txnextfrmifg value. When this is "
     "set, the value from ff_txnextfrmifg is used as the IFG between frames. "
     "When this is not set, the value programmed in IFG Length is used for all "
     "frames."},

    {"enautodrnonflt",
     15,
     15,
     0,
     10934,
     "Enables Auto Draining of Frames from FIFO Interface when the MAC "
     "receiver is in Fault state."},

};
reg_decoder_t mcmac2_txconfig_flds = {8, mcmac2_txconfig_fld_list, 16};

reg_decoder_fld_t mcmac2_rxconfig_fld_list[] = {
    {"disfcschk",
     0,
     0,
     0,
     10935,
     "When set, the UMAC doesn't perform FCS Checking on the incoming frames "
     "for this Channel and the CRC Error status is not set for any of the "
     "frames. When reset, the UMAC operates normally and checks FCS on every "
     "incoming frame, for this Channel. Frames with CRC Errors are reported in "
     "the Frame Status."},

    {"stripfcs",
     1,
     1,
     0,
     10936,
     "When set, the FCS field is stripped of the frame before the Frame is "
     "transferred to the Application logic for this Channel's frames. The "
     "Frame Length field is updated to reflect the new length (without the FCS "
     "field). When reset, the FCS field is not stripped and is part of the "
     "frame that is transferred to Application logic.   Irrespective of the "
     "Strip FCS field, FCS Checking is performed on every frame unless Disable "
     "FCS Check is set. When theFCS field is checked, the CRC Error status is "
     "reported for every frame. "},

    {"vlanchk",
     3,
     2,
     0,
     10937,
     "Per the IEEE 802.3 specification, the minFrameSize and maxFrameSize "
     "values for untagged frames are set to 64 bytes and 1518 bytes "
     "respectively. The values for tagged (VLAN Tagged) frame sizes are "
     "updated to account for 4-bytes of VLAN Tag and the maxFrameSize value is "
     "updated to 1522 Bytes.   The UMAC supports up to 3 VLAN Tags per frame. "
     "It can adjust the maxFrameSize field to support up to 3 VLAN Tags (12 "
     "Bytes), based on the number of VLAN tags that are present in the "
     "incoming frame. This helps in false MaxFrameSize violation reporting for "
     "frames with VLAN tags that exceed 1518 bytes but are under the "
     "1518+tagbytes in length.  The UMAC will use the value programmed in this "
     "field to determine the number of VLAN's for which the MAC will check "
     "(for this Channel) and update the MaxFrameSize field accordingly.  "
     "2'b00: Disable VLAN Checking. 2'b01: Check for up to 1 VLAN Tag (Use the "
     "VLAN Tag#1 Register) 2'b10: Check for up to 2 VLAN Tags (Use VLAN Tag#1 "
     "and #2 Registers) 2'b11: Check for up to 3 VLAN Tags (Use VLAN Tag#1, "
     "#2, and #3Registers)"},

    {"promiscuous",
     4,
     4,
     0,
     10938,
     "When set, the UMAC disables address checking on all frames that are "
     "received on this Channel, and all the frames received on this channel "
     "are passed onto the FIFO Interface. The Frame Status reflects the status "
     "of the Address Checking.  When the bit is cleared, the DA field is "
     "compared against the contents of the MAC Address Register (for Unicast "
     "Frames) and the Multicast Hash table (for Multicast Frames). Frames that "
     "fail topass the Address filtering will be dropped and not transferred to "
     "the FIFO Interface  "},

    {"enrxfcdec",
     5,
     5,
     0,
     10939,
     "When set, the UMAC Core is enabled for Flow-Control decode operation for "
     "this Channel and it will decode all the incoming frames for PAUSE "
     "Control Frames as specified in the IEEE 802.3 Specification.   If the "
     "UMAC Core receives a valid PAUSE Control Frame, it will disable the "
     "transmission of user data frames for the time given in the PAUSE_TIME "
     "field of the PAUSE Control Frame.  If the UMAC Core receives a valid "
     "Priority PAUSE Control Frame, it will load the timers and providethe "
     "XOFF indication to the Application logic (ff_rxch0pfcxoff[7:0]) based on "
     "the Time Vector fields in the 8 priorities.  When reset, the "
     "Flow-Control Operation in the UMAC is disabled for this Channel, and it "
     "does not decode the frames for PAUSE Control Frames or Priority Pause "
     "Flow Control Frames."},

    {"prelength",
     7,
     7,
     0,
     10940,
     "This defines the START/PREAMBLE/SFD length that is expected of all the "
     "frames received on this Channel.  1'b0: Default 8-bytes (START, 6 bytes "
     "of PREAMBLE, 1 byte of SFD) 1'b1: 4-bytes (START, 2 bytes of PREAMBLE, 1 "
     "byte of SFD).  This is applicable when the Channel is operating at 10G "
     "rates and above."},

    {"filterpf",
     8,
     8,
     0,
     10941,
     "When this bit is set, the UMAC will filter out the frames with DA "
     "matching the reserved Multicast PAUSE Control Frame address "
     "(01:80:C2:00:00:01) from being sent to the Application Logic.   When "
     "this bit is reset, the UMAC does not filter out the frames with the DA "
     "matching the reserved Multicast PAUSE Control Frame address "
     "(01:80:C2:00:00:01).  This bit has no effect on actual "
     "processing/decoding on the PAUSE (Priority PAUSE) Control frames, which "
     "is controlled thru theEnable Rx Flow Control Decode bit.  When the PAUSE "
     "control frames are passed to the application logic, (Filter PAUSE "
     "Frames) is not set, but the application logic can still identify the "
     "PAUSE Control frames using Bit[31] in ff_rxsts that is sent out with the "
     "frame."},

    {"enearlyeofdet",
     9,
     9,
     0,
     10942,
     "This should be set only when the UMAC is configured for 4x10G mode and "
     "the Low Latency option is selected in the PCS. This enables the "
     "detection of Early EOF indication from PCS which helps in reducing the "
     "latency.  Note: This is to be set only when the UMAC is operating in "
     "4x10G Mode. Setting this in other modes will cause the UMAC to have "
     "unexpected behavior."},

};
reg_decoder_t mcmac2_rxconfig_flds = {8, mcmac2_rxconfig_fld_list, 16};

reg_decoder_fld_t mcmac2_maxfrmsize_fld_list[] = {
    {"maxfrmsize",
     15,
     0,
     0,
     10943,
     "This field determines the Maximum Frame Size for untagged frames that is "
     "used in checking the MaxFrameLength violations on this Channel. For "
     "frames larger than this value, the MaxFrameLength Error bit is set in "
     "the Frame Status."},

};
reg_decoder_t mcmac2_maxfrmsize_flds = {1, mcmac2_maxfrmsize_fld_list, 16};

reg_decoder_fld_t mcmac2_maxtxjabsize_fld_list[] = {
    {"maxtxjabsize",
     15,
     0,
     0,
     10944,
     "This field determines the Jabber Size for the outgoing (Transmit) frames "
     "on this Channel. When the length of the current outgoing frame on this "
     "Channel exceeds the value programmed in this field, the Frame is "
     "considered a Jabber Frame and is truncated at that point with EOF-ERROR. "
     "The PHY will eventually force ERROR code onto the line. This will limit "
     "the frame transmission run-off in case of Application logic error.  This "
     "value has to be a multiple of the Data path width (in bytes) inside the "
     "MAC , per the  following: 100G: Multiple of 16 40G: Multiple of 8 10G: "
     "Multiple of 4 1G: Multiple of 1."},

};
reg_decoder_t mcmac2_maxtxjabsize_flds = {1, mcmac2_maxtxjabsize_fld_list, 16};

reg_decoder_fld_t mcmac2_maxrxjabsize_fld_list[] = {
    {"maxrxjabsize",
     15,
     0,
     0,
     10945,
     "This field determines the Jabber Size for the incoming (Receive) frames "
     "on this Channel. When the length of the current incoming frame on this "
     "Channel equals or exceeds the value programmed in this field, the Frame "
     "is considered a Jabber Frame and istruncated at that point. The Frame "
     "Status is updated with a Jabber Error and the rest of the Jabbered frame "
     "that is being received is ignored.   This value has to be a multiple of "
     "the Data path width (in bytes) inside the MAC,per the following: 100G: "
     "Multiple of 16 40G: Multiple of 8 10G: Multiple of 4 1G: Multiple of 1."},

};
reg_decoder_t mcmac2_maxrxjabsize_flds = {1, mcmac2_maxrxjabsize_fld_list, 16};

reg_decoder_fld_t mcmac2_txpfcvec_fld_list[] = {
    {"txpfcvec",
     7,
     0,
     0,
     10946,
     "Each bit in this field determines whether the Priority is enabled or "
     "disabled for monitoring PFC transmission. This field is also transmitted "
     "in the Priority Vector field in the PFC Pause Frame.  "},

};
reg_decoder_t mcmac2_txpfcvec_flds = {1, mcmac2_txpfcvec_fld_list, 16};

reg_decoder_fld_t mcmac2_vlantag1_fld_list[] = {
    {"vlantag1",
     15,
     0,
     0,
     10947,
     "This field is used to identify the first VLAN Tag in the incoming frames "
     "on this Channel. This helps in programming the outer VLAN Tag based on "
     "Application requirements."},

};
reg_decoder_t mcmac2_vlantag1_flds = {1, mcmac2_vlantag1_fld_list, 16};

reg_decoder_fld_t mcmac2_vlantag2_fld_list[] = {
    {"vlantag2",
     15,
     0,
     0,
     10948,
     "This field is used to identify the second VLAN Tag in the incoming "
     "frames on this Channel. This helps in programming the inner VLAN Tag "
     "based on Application requirements."},

};
reg_decoder_t mcmac2_vlantag2_flds = {1, mcmac2_vlantag2_fld_list, 16};

reg_decoder_fld_t mcmac2_vlantag3_fld_list[] = {
    {"vlantag3",
     15,
     0,
     0,
     10949,
     "This field is used to identify the third VLAN Tag in the incoming frames "
     "on this Channel. This helps in programming the innermost VLAN Tag based "
     "on Application requirements."},

};
reg_decoder_t mcmac2_vlantag3_flds = {1, mcmac2_vlantag3_fld_list, 16};

reg_decoder_fld_t mcmac2_macaddrlo_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     10950,
     "This byte contains the first Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac2_macaddrlo_flds = {1, mcmac2_macaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac2_macaddrmid_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     10951,
     "This byte contains the third Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac2_macaddrmid_flds = {1, mcmac2_macaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac2_macaddrhi_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     10952,
     "This byte contains the fifth Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac2_macaddrhi_flds = {1, mcmac2_macaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac2_mchasht1_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10953,
     "This field contains the bits[15:0] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac2_mchasht1_flds = {1, mcmac2_mchasht1_fld_list, 16};

reg_decoder_fld_t mcmac2_mchasht2_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10954,
     "This field contains the bits[31:16] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac2_mchasht2_flds = {1, mcmac2_mchasht2_fld_list, 16};

reg_decoder_fld_t mcmac2_mchasht3_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10955,
     "This field contains the bits[47:32] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac2_mchasht3_flds = {1, mcmac2_mchasht3_fld_list, 16};

reg_decoder_fld_t mcmac2_mchasht4_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     10956,
     "This field contains the bits[63:48] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac2_mchasht4_flds = {1, mcmac2_mchasht4_fld_list, 16};

reg_decoder_fld_t mcmac2_fcfrmgen_fld_list[] = {
    {"fcfrmgen",
     0,
     0,
     0,
     10957,
     "This is set by Software for the UMAC to transmit the PAUSE Control frame "
     "on this Channel. When this bit is set, the UMAC transmits the PAUSE "
     "control frame by using the values programmed in various PAUSE Control "
     "specific registers (SA/PAUSE_TIME) on this Channel. Once the PAUSE "
     "Control frame is transmitted, the bit is cleared by the UMAC.   The "
     "following are the steps for generating the PAUSE Control frame: 1) Read "
     "and Verify this bit is cleared. If not cleared wait until the bit is "
     "cleared (polling) 2) Program the Pause Frame SA and PAUSE_TIME registers "
     "3) Set this bit. 4) Poll this bit until the bit is cleared."},

};
reg_decoder_t mcmac2_fcfrmgen_flds = {1, mcmac2_fcfrmgen_fld_list, 16};

reg_decoder_fld_t mcmac2_fcdaddrlo_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     10958,
     "This byte contains the first Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac2_fcdaddrlo_flds = {1, mcmac2_fcdaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac2_fcdaddrmid_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     10959,
     "This byte contains the third Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac2_fcdaddrmid_flds = {1, mcmac2_fcdaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac2_fcdaddrhi_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     10960,
     "This byte contains the fifth Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac2_fcdaddrhi_flds = {1, mcmac2_fcdaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac2_fcsaddrlo_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     10961,
     "This byte contains the first Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac2_fcsaddrlo_flds = {1, mcmac2_fcsaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac2_fcsaddrmid_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     10962,
     "This byte contains the third Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac2_fcsaddrmid_flds = {1, mcmac2_fcsaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac2_fcsaddrhi_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     10963,
     "This byte contains the fifth Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac2_fcsaddrhi_flds = {1, mcmac2_fcsaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac2_fcpausetime_fld_list[] = {
    {"fcpausetime",
     15,
     0,
     0,
     10964,
     "This contains the Pause-Time value that is used in the generated PAUSE "
     "Control Frame under software control for this Channel. The 16-bit value "
     "is sent out as defined in the IEEE 802.3 Specification with most "
     "significant byte first followed by least significant byte."},

};
reg_decoder_t mcmac2_fcpausetime_flds = {1, mcmac2_fcpausetime_fld_list, 16};

reg_decoder_fld_t mcmac2_xoffpausetime_fld_list[] = {
    {"xoffpausetime",
     15,
     0,
     0,
     10965,
     "This field contains the Pause-Time value that is used in the generated "
     "PAUSE Control Frame when the XOFF PAUSE Frame is to be transmitted. This "
     "field is used both in the regular PAUSE flow Control Frames and Priority "
     "PAUSE Flow Control Frames. The 16-bit value is sent out as defined in "
     "the IEEE 802.3 Specification with most significant byte first followed "
     "by least significant byte"},

};
reg_decoder_t mcmac2_xoffpausetime_flds = {
    1, mcmac2_xoffpausetime_fld_list, 16};

reg_decoder_fld_t mcmac2_xonpausetime_fld_list[] = {
    {"xonpausetime",
     15,
     0,
     0,
     10966,
     "This field contains the Pause-Time value that is used in the generated "
     "PAUSE Control Frame when the XON PAUSE Frame is to be transmitted. This "
     "field is used both in the regular PAUSE flow Control Frames and Priority "
     "PAUSE Flow Control Frames. The 16-bit value is sent out as defined in "
     "the IEEE 802.3 Specification with most significant byte first followed "
     "by least significant byte"},

};
reg_decoder_t mcmac2_xonpausetime_flds = {1, mcmac2_xonpausetime_fld_list, 16};

reg_decoder_fld_t mcmac2_txtsinfo_fld_list[] = {
    {"tsid",
     1,
     0,
     0,
     10967,
     "This field contains the Transmit Timestamp ID associated with the Time "
     "Stamp that is present in the TimeStamp Value Register."},

    {"tsvld",
     15,
     15,
     0,
     10968,
     "This bit indicates whether the Transmit Timestamp Value Register has "
     "valid Value or not. This bit is set when the Timestamp Value register "
     "has been loaded with a new Timestamp Value from the TimeStamp FIFO."},

};
reg_decoder_t mcmac2_txtsinfo_flds = {2, mcmac2_txtsinfo_fld_list, 16};

reg_decoder_fld_t mcmac2_tsv0_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10969,
     "This field contains the bits [15:0] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac2_tsv0_flds = {1, mcmac2_tsv0_fld_list, 16};

reg_decoder_fld_t mcmac2_tsv1_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10970,
     "This field contains the bits [31:16] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac2_tsv1_flds = {1, mcmac2_tsv1_fld_list, 16};

reg_decoder_fld_t mcmac2_tsv2_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10971,
     "This field contains the bits [47:32] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac2_tsv2_flds = {1, mcmac2_tsv2_fld_list, 16};

reg_decoder_fld_t mcmac2_tsv3_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     10972,
     "This field contains the bits [63:48] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac2_tsv3_flds = {1, mcmac2_tsv3_fld_list, 16};

reg_decoder_fld_t mcmac2_txtsdelta_fld_list[] = {
    {"txtsdelta",
     15,
     0,
     0,
     10973,
     "This is the delta value that is subtracted from the latched Tx Timestamp "
     "Value before it is passed along with the Frame SOF."},

};
reg_decoder_t mcmac2_txtsdelta_flds = {1, mcmac2_txtsdelta_fld_list, 16};

reg_decoder_fld_t mcmac2_rxtsdelta_fld_list[] = {
    {"rxtsdelta",
     15,
     0,
     0,
     10974,
     "This is the delta value that is subtracted from the latched Rx Timestamp "
     "Value before it is passed along with the Frame SOF."},

};
reg_decoder_t mcmac2_rxtsdelta_flds = {1, mcmac2_rxtsdelta_fld_list, 16};

reg_decoder_fld_t mcmac2_minframesize_fld_list[] = {
    {"minframesize",
     7,
     0,
     0,
     10975,
     "This field is used to determine the MinFrameSize in Transmit and Receive "
     "operation.  The MinFrameSize should be programmed with a value which is "
     "multiple of 4 and with a minimum value of 32."},

};
reg_decoder_t mcmac2_minframesize_flds = {1, mcmac2_minframesize_fld_list, 16};

reg_decoder_fld_t mcmac2_txvlantag_fld_list[] = {
    {"txvlantag",
     15,
     0,
     0,
     10976,
     "This defines the VLAN Tag that is used for comparing and identifying "
     "VLAN frames on Transmit Side. "},

};
reg_decoder_t mcmac2_txvlantag_flds = {1, mcmac2_txvlantag_fld_list, 16};

reg_decoder_fld_t mcmac2_fint0_fld_list[] = {
    {"ovrdint10", 1, 0, 0, 10977, "TX underrun Interrupt override"},

};
reg_decoder_t mcmac2_fint0_flds = {1, mcmac2_fint0_fld_list, 16};

reg_decoder_fld_t mcmac2_fint1_fld_list[] = {
    {"ovrdint32", 1, 0, 0, 10978, "TX timestamp fifo overflow override"},

};
reg_decoder_t mcmac2_fint1_flds = {1, mcmac2_fint1_fld_list, 16};

reg_decoder_fld_t mcmac2_fint2_fld_list[] = {
    {"ovrdint654", 2, 0, 0, 10979, "RX Local Fault Interrupt override"},

};
reg_decoder_t mcmac2_fint2_flds = {1, mcmac2_fint2_fld_list, 16};

reg_decoder_fld_t mcmac2_slotclkcnt_fld_list[] = {
    {"slotclkcnt",
     7,
     0,
     0,
     10980,
     "When this field is set to zero, the transmit and receive MAC uses "
     "internal DataValid/DataReady to count the valid clocks for the Slot "
     "Timer Logic. In this mode the Slot is 512 Bit times based on the line "
     "rate.  When this field is non-zero, this field determines the number of "
     "Clocks (Core_clk_i) for the Slot Timer.   The Slot Timer is used to "
     "decrement the PAUSE Quanta."},

};
reg_decoder_t mcmac2_slotclkcnt_flds = {1, mcmac2_slotclkcnt_fld_list, 16};

reg_decoder_fld_t mcmac2_txdebug_fld_list[] = {
    {"stompeoperr",
     0,
     0,
     0,
     10981,
     "When set the EOP/ERR is ignored by the MAC."},

    {"txlfault",
     1,
     1,
     0,
     10982,
     "When set, the MAC continuously transmits L_FAULT ordered set."},

    {"txrfault",
     2,
     2,
     0,
     10983,
     "When set, the MAC continuously transmits R_FAULT ordered set."},

    {"txidle",
     3,
     3,
     0,
     10984,
     "When set, the MAC continuously transmits IDLE Pattern."},

    {"txtestpattern",
     4,
     4,
     0,
     10985,
     "This should be set when the PCS is programmed to transmit Test Pattern. "
     "This bit does not have any functionality in the MAC other than in the "
     "generation of TX_GOOD signal output. "},

    {"forcerxxoff",
     5,
     5,
     0,
     10986,
     "When set, the XOFF (for all priorities) is forced on APP_FIFO Interface "
     "when a regular PAUSE frame is received and the Transmit MAC is not "
     "disabled. When clear, normal PAUSE frame reception behavior of disabling "
     "TxMAC"},

    {"txautodraintxdisable",
     6,
     6,
     0,
     10987,
     "TxAutoDrainonTxDisable: When set, the TxAutoDrain function is enabled "
     "when the TXMAC is disabled (TXEnable is set to 1'b0). When clear, the "
     "frames are not drained when the MAC is disabled."},

    {"macdebug0",
     7,
     7,
     0,
     10988,
     "MAC Debug #0. This should be treated as reserved and not modified by "
     "User."},

    {"txvlantagena",
     8,
     8,
     0,
     10989,
     "This enables the VLAN Tag comparison and identification of VLAN frames "
     "on Transmit Side"},

    {"ovrdtxdisonpause",
     9,
     9,
     0,
     10990,
     "When set, the TxDisable on RxPause function is overridden. The Tx Is not "
     "disabled, but the signal ff_Rxfcxoff signal is generated on the APP_FIFO "
     "Interface. When cleared, the normal Pause function is enabled where the "
     "TXMAC is disabled for the duration of PAUSE Time."},

};
reg_decoder_t mcmac2_txdebug_flds = {10, mcmac2_txdebug_fld_list, 16};

reg_decoder_fld_t mcmac3_ctrl_fld_list[] = {
    {"txenable",
     0,
     0,
     0,
     10991,
     "When set, this Channel's MAC transmitter is enabled and it will transmit "
     "frames from the Transmit FIFO/Application logic on to the PCS Interface. "
     "When reset, this Channel's MAC's transmitter is disabled and will not "
     "transmit any frames. "},

    {"rxenable",
     1,
     1,
     0,
     10992,
     "When set, this Channel's MAC receiver is enabled and it will receive "
     "frames from the PCS and transfer them to Receive FIFO/Application logic. "
     "When reset, this Channel's MAC Core receiver is disabled and will not "
     "receive any frames."},

    {"swreset",
     2,
     2,
     0,
     10993,
     "Setting this bit resets this Channel's various MAC logic to the default "
     "state. The contents of the Configuration Registers related to this "
     "Channel are not affected by this bit.  This Channel's MAC logic is "
     "continued to be in reset state while this bit is set."},

    {"maclpbk",
     3,
     3,
     0,
     10994,
     "Setting this bit enables the Loopback on the MAC-PCS Interface for this "
     "Channel. The Transmit data and controls are looped back on to the "
     "receive MAC module for this Channel.  When this loopback is enabled, the "
     "data received from the PCS for this channel is ignored."},

    {"faultovrd",
     4,
     4,
     0,
     10995,
     "When set, overrides the Ch#0 Link Fault Status. For diagnostics only"},

    {"txdrn",
     5,
     5,
     0,
     10996,
     "Setting this bit causes transmit path to enter into Drain mode where all "
     "the data from the TXFIFO is drained out."},

};
reg_decoder_t mcmac3_ctrl_flds = {6, mcmac3_ctrl_fld_list, 16};

reg_decoder_fld_t mcmac3_txconfig_fld_list[] = {
    {"disfcs",
     0,
     0,
     0,
     10997,
     "When set, the FCS calculation and insertion logic is disabled in the "
     "transmit path for this Channel. FCS insertion is disabled for all the "
     "frames. When reset, the FCS is calculated on all frames and insertion is "
     "based on the per frame control signal (ff_txchdisfcs for this Channel).  "
     "When the Disable FCS Insertion is set, it is expected that the frames "
     "coming in from the Application logic will meet the 64-byte MinFrameSize "
     "requirements and contains the FCS field to make the outgoing frame IEEE "
     "802.3 specification Compliant."},

    {"invfcs",
     1,
     1,
     0,
     10998,
     "When set, the UMAC inverts the FCS field that is being inserted into the "
     "outgoing frames for this Channel. When reset, the UMAC Core operates "
     "normally for FCS insertion for this Channel.   Note: According to the "
     "IEEE 802.3 specification, the FCS is calculated beginning from the first "
     "byte of DA until the last byte of the DATA/PAD field. The calculated FCS "
     "is then inverted and sent out MSB first. So in normal mode, the "
     "calculated FCS is inverted before being inserted into the outgoing "
     "frame. When the Invert FCS is set, the FCS field is double inverted (No "
     "inversion happens)."},

    {"ifglength",
     7,
     2,
     0,
     10999,
     "This determines the minimum IFG (Inter Frame Gap) in Bytes, to be "
     "inserted between outgoing frames for this Channel, when the Application "
     "logic has back to back frames, and no Frame Pacing is defined. The IEEE "
     "802.3 specification specifies a minimum IFG of 96 bit-times between "
     "frames. The following are the permissible values for the IFG Length "
     "field.   For 10M, 100M, and 1000M, the range is as follows. Minimum: 1 "
     "Maximum: 63.  For 10G the range is as follows: Minimum: 8 Maximum: 63.  "
     "For 40G, 100G the range is as follows: Minimum: 12 Maximum: 63.  In case "
     "of 10G speed and above, the Deficit IDLE Counter is implemented to "
     "maintain the average rate of the programmed IFG.  The "
     "ff_txnextfrifg[7:0] value temporarily overrides the IFG programmed in "
     "this register.  When the IFG Length field is programmed with Zero, then "
     "the UMAC operates in Zero length IFG mode where the frames are "
     "transmitted back to back while horning the START alignment to Lane#0."},

    {"fcfrmgen",
     8,
     8,
     0,
     11000,
     "When set, this Channel is enabled for transmission of PAUSE Control "
     "Frames. The PAUSE Control frames are transmitted either on Software "
     "control or based on Application logic controls. The parameters for the "
     "generated PAUSE Frame are configured into the registers. When reset, "
     "this Channel does not generate a PAUSE Control Frame."},

    {"pfcfrmgen",
     9,
     9,
     0,
     11001,
     "When set, this Channel is enabled for transmission of PFC Control "
     "Frames. The PFC Control frames are transmitted based on Application "
     "logic's controls. When reset, this Channel doesn't generate priority "
     "PAUSE Control Frame."},

    {"prelength",
     13,
     11,
     0,
     11002,
     "This defines length of the Preamble that is used for every outgoing "
     "frame for this Channel.  Based on the MAC's operating speed as specified "
     "below:  10G Rates and above - 3'b000: Default 8-bytes (START, 6 bytes of "
     "PRE, 1 byte of SFD) - 3'b001: Reserved - 3'b010: Reserved - 3'b011: "
     "Reserved - 3'b100: bytes (START, 2 bytes of PRE, 1 byte of SFD). - "
     "3'b101: Reserved - 3'b111: Reserved - 3'b111: Reserved  1G Rates and "
     "below - 3'b000: Default 8-bytes (7 Bytes of PRE plus 1 Byte of SFD) - "
     "3'b001: 1 Byte of SFD only. - 3'b010: 1 Byte of PRE plus 1 Byte of SFD - "
     "3'b011: 2 Bytes of PRE plus 1 Byte of SFD - 3'b100: 3 Bytes of PRE plus "
     "1 Byte of SFD - 3'b101: 4 Bytes of PRE plus 1 Byte of SFD - 3'b110: 5 "
     "Bytes of PRE plus 1 Byte of SFD - 3'b111: 6 Bytes of PRE plus 1 Byte of "
     "SFD"},

    {"enfrmpace",
     14,
     14,
     0,
     11003,
     "Enable Pacing of Frames using the ff_txnextfrmifg value. When this is "
     "set, the value from ff_txnextfrmifg is used as the IFG between frames. "
     "When this is not set, the value programmed in IFG Length is used for all "
     "frames."},

    {"enautodrnonflt",
     15,
     15,
     0,
     11004,
     "Enables Auto Draining of Frames from FIFO Interface when the MAC "
     "receiver is in Fault state."},

};
reg_decoder_t mcmac3_txconfig_flds = {8, mcmac3_txconfig_fld_list, 16};

reg_decoder_fld_t mcmac3_rxconfig_fld_list[] = {
    {"disfcschk",
     0,
     0,
     0,
     11005,
     "When set, the UMAC doesn't perform FCS Checking on the incoming frames "
     "for this Channel and the CRC Error status is not set for any of the "
     "frames. When reset, the UMAC operates normally and checks FCS on every "
     "incoming frame, for this Channel. Frames with CRC Errors are reported in "
     "the Frame Status."},

    {"stripfcs",
     1,
     1,
     0,
     11006,
     "When set, the FCS field is stripped of the frame before the Frame is "
     "transferred to the Application logic for this Channel's frames. The "
     "Frame Length field is updated to reflect the new length (without the FCS "
     "field). When reset, the FCS field is not stripped and is part of the "
     "frame that is transferred to Application logic.   Irrespective of the "
     "Strip FCS field, FCS Checking is performed on every frame unless Disable "
     "FCS Check is set. When theFCS field is checked, the CRC Error status is "
     "reported for every frame. "},

    {"vlanchk",
     3,
     2,
     0,
     11007,
     "Per the IEEE 802.3 specification, the minFrameSize and maxFrameSize "
     "values for untagged frames are set to 64 bytes and 1518 bytes "
     "respectively. The values for tagged (VLAN Tagged) frame sizes are "
     "updated to account for 4-bytes of VLAN Tag and the maxFrameSize value is "
     "updated to 1522 Bytes.   The UMAC supports up to 3 VLAN Tags per frame. "
     "It can adjust the maxFrameSize field to support up to 3 VLAN Tags (12 "
     "Bytes), based on the number of VLAN tags that are present in the "
     "incoming frame. This helps in false MaxFrameSize violation reporting for "
     "frames with VLAN tags that exceed 1518 bytes but are under the "
     "1518+tagbytes in length.  The UMAC will use the value programmed in this "
     "field to determine the number of VLAN's for which the MAC will check "
     "(for this Channel) and update the MaxFrameSize field accordingly.  "
     "2'b00: Disable VLAN Checking. 2'b01: Check for up to 1 VLAN Tag (Use the "
     "VLAN Tag#1 Register) 2'b10: Check for up to 2 VLAN Tags (Use VLAN Tag#1 "
     "and #2 Registers) 2'b11: Check for up to 3 VLAN Tags (Use VLAN Tag#1, "
     "#2, and #3Registers)"},

    {"promiscuous",
     4,
     4,
     0,
     11008,
     "When set, the UMAC disables address checking on all frames that are "
     "received on this Channel, and all the frames received on this channel "
     "are passed onto the FIFO Interface. The Frame Status reflects the status "
     "of the Address Checking.  When the bit is cleared, the DA field is "
     "compared against the contents of the MAC Address Register (for Unicast "
     "Frames) and the Multicast Hash table (for Multicast Frames). Frames that "
     "fail topass the Address filtering will be dropped and not transferred to "
     "the FIFO Interface  "},

    {"enrxfcdec",
     5,
     5,
     0,
     11009,
     "When set, the UMAC Core is enabled for Flow-Control decode operation for "
     "this Channel and it will decode all the incoming frames for PAUSE "
     "Control Frames as specified in the IEEE 802.3 Specification.   If the "
     "UMAC Core receives a valid PAUSE Control Frame, it will disable the "
     "transmission of user data frames for the time given in the PAUSE_TIME "
     "field of the PAUSE Control Frame.  If the UMAC Core receives a valid "
     "Priority PAUSE Control Frame, it will load the timers and providethe "
     "XOFF indication to the Application logic (ff_rxch0pfcxoff[7:0]) based on "
     "the Time Vector fields in the 8 priorities.  When reset, the "
     "Flow-Control Operation in the UMAC is disabled for this Channel, and it "
     "does not decode the frames for PAUSE Control Frames or Priority Pause "
     "Flow Control Frames."},

    {"prelength",
     7,
     7,
     0,
     11010,
     "This defines the START/PREAMBLE/SFD length that is expected of all the "
     "frames received on this Channel.  1'b0: Default 8-bytes (START, 6 bytes "
     "of PREAMBLE, 1 byte of SFD) 1'b1: 4-bytes (START, 2 bytes of PREAMBLE, 1 "
     "byte of SFD).  This is applicable when the Channel is operating at 10G "
     "rates and above."},

    {"filterpf",
     8,
     8,
     0,
     11011,
     "When this bit is set, the UMAC will filter out the frames with DA "
     "matching the reserved Multicast PAUSE Control Frame address "
     "(01:80:C2:00:00:01) from being sent to the Application Logic.   When "
     "this bit is reset, the UMAC does not filter out the frames with the DA "
     "matching the reserved Multicast PAUSE Control Frame address "
     "(01:80:C2:00:00:01).  This bit has no effect on actual "
     "processing/decoding on the PAUSE (Priority PAUSE) Control frames, which "
     "is controlled thru theEnable Rx Flow Control Decode bit.  When the PAUSE "
     "control frames are passed to the application logic, (Filter PAUSE "
     "Frames) is not set, but the application logic can still identify the "
     "PAUSE Control frames using Bit[31] in ff_rxsts that is sent out with the "
     "frame."},

    {"enearlyeofdet",
     9,
     9,
     0,
     11012,
     "This should be set only when the UMAC is configured for 4x10G mode and "
     "the Low Latency option is selected in the PCS. This enables the "
     "detection of Early EOF indication from PCS which helps in reducing the "
     "latency.  Note: This is to be set only when the UMAC is operating in "
     "4x10G Mode. Setting this in other modes will cause the UMAC to have "
     "unexpected behavior."},

};
reg_decoder_t mcmac3_rxconfig_flds = {8, mcmac3_rxconfig_fld_list, 16};

reg_decoder_fld_t mcmac3_maxfrmsize_fld_list[] = {
    {"maxfrmsize",
     15,
     0,
     0,
     11013,
     "This field determines the Maximum Frame Size for untagged frames that is "
     "used in checking the MaxFrameLength violations on this Channel. For "
     "frames larger than this value, the MaxFrameLength Error bit is set in "
     "the Frame Status."},

};
reg_decoder_t mcmac3_maxfrmsize_flds = {1, mcmac3_maxfrmsize_fld_list, 16};

reg_decoder_fld_t mcmac3_maxtxjabsize_fld_list[] = {
    {"maxtxjabsize",
     15,
     0,
     0,
     11014,
     "This field determines the Jabber Size for the outgoing (Transmit) frames "
     "on this Channel. When the length of the current outgoing frame on this "
     "Channel exceeds the value programmed in this field, the Frame is "
     "considered a Jabber Frame and is truncated at that point with EOF-ERROR. "
     "The PHY will eventually force ERROR code onto the line. This will limit "
     "the frame transmission run-off in case of Application logic error.  This "
     "value has to be a multiple of the Data path width (in bytes) inside the "
     "MAC , per the  following: 100G: Multiple of 16 40G: Multiple of 8 10G: "
     "Multiple of 4 1G: Multiple of 1."},

};
reg_decoder_t mcmac3_maxtxjabsize_flds = {1, mcmac3_maxtxjabsize_fld_list, 16};

reg_decoder_fld_t mcmac3_maxrxjabsize_fld_list[] = {
    {"maxrxjabsize",
     15,
     0,
     0,
     11015,
     "This field determines the Jabber Size for the incoming (Receive) frames "
     "on this Channel. When the length of the current incoming frame on this "
     "Channel equals or exceeds the value programmed in this field, the Frame "
     "is considered a Jabber Frame and istruncated at that point. The Frame "
     "Status is updated with a Jabber Error and the rest of the Jabbered frame "
     "that is being received is ignored.   This value has to be a multiple of "
     "the Data path width (in bytes) inside the MAC,per the following: 100G: "
     "Multiple of 16 40G: Multiple of 8 10G: Multiple of 4 1G: Multiple of 1."},

};
reg_decoder_t mcmac3_maxrxjabsize_flds = {1, mcmac3_maxrxjabsize_fld_list, 16};

reg_decoder_fld_t mcmac3_txpfcvec_fld_list[] = {
    {"txpfcvec",
     7,
     0,
     0,
     11016,
     "Each bit in this field determines whether the Priority is enabled or "
     "disabled for monitoring PFC transmission. This field is also transmitted "
     "in the Priority Vector field in the PFC Pause Frame.  "},

};
reg_decoder_t mcmac3_txpfcvec_flds = {1, mcmac3_txpfcvec_fld_list, 16};

reg_decoder_fld_t mcmac3_vlantag1_fld_list[] = {
    {"vlantag1",
     15,
     0,
     0,
     11017,
     "This field is used to identify the first VLAN Tag in the incoming frames "
     "on this Channel. This helps in programming the outer VLAN Tag based on "
     "Application requirements."},

};
reg_decoder_t mcmac3_vlantag1_flds = {1, mcmac3_vlantag1_fld_list, 16};

reg_decoder_fld_t mcmac3_vlantag2_fld_list[] = {
    {"vlantag2",
     15,
     0,
     0,
     11018,
     "This field is used to identify the second VLAN Tag in the incoming "
     "frames on this Channel. This helps in programming the inner VLAN Tag "
     "based on Application requirements."},

};
reg_decoder_t mcmac3_vlantag2_flds = {1, mcmac3_vlantag2_fld_list, 16};

reg_decoder_fld_t mcmac3_vlantag3_fld_list[] = {
    {"vlantag3",
     15,
     0,
     0,
     11019,
     "This field is used to identify the third VLAN Tag in the incoming frames "
     "on this Channel. This helps in programming the innermost VLAN Tag based "
     "on Application requirements."},

};
reg_decoder_t mcmac3_vlantag3_flds = {1, mcmac3_vlantag3_fld_list, 16};

reg_decoder_fld_t mcmac3_macaddrlo_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     11020,
     "This byte contains the first Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac3_macaddrlo_flds = {1, mcmac3_macaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac3_macaddrmid_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     11021,
     "This byte contains the third Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac3_macaddrmid_flds = {1, mcmac3_macaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac3_macaddrhi_fld_list[] = {
    {"macaddr",
     15,
     0,
     0,
     11022,
     "This byte contains the fifth Byte of this Channel's MAC Address as it "
     "appears on the Receiving Frame in the DA Field. This is used for Address "
     "filtering when enabled."},

};
reg_decoder_t mcmac3_macaddrhi_flds = {1, mcmac3_macaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac3_mchasht1_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     11023,
     "This field contains the bits[15:0] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac3_mchasht1_flds = {1, mcmac3_mchasht1_fld_list, 16};

reg_decoder_fld_t mcmac3_mchasht2_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     11024,
     "This field contains the bits[31:16] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac3_mchasht2_flds = {1, mcmac3_mchasht2_fld_list, 16};

reg_decoder_fld_t mcmac3_mchasht3_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     11025,
     "This field contains the bits[47:32] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac3_mchasht3_flds = {1, mcmac3_mchasht3_fld_list, 16};

reg_decoder_fld_t mcmac3_mchasht4_fld_list[] = {
    {"mchash",
     15,
     0,
     0,
     11026,
     "This field contains the bits[63:48] of the 64-bit Multicast Hash "
     "Filtering Table. This table is used to perform Multicast Hash filtering "
     "for the frames received on this Channel."},

};
reg_decoder_t mcmac3_mchasht4_flds = {1, mcmac3_mchasht4_fld_list, 16};

reg_decoder_fld_t mcmac3_fcfrmgen_fld_list[] = {
    {"fcfrmgen",
     0,
     0,
     0,
     11027,
     "This is set by Software for the UMAC to transmit the PAUSE Control frame "
     "on this Channel. When this bit is set, the UMAC transmits the PAUSE "
     "control frame by using the values programmed in various PAUSE Control "
     "specific registers (SA/PAUSE_TIME) on this Channel. Once the PAUSE "
     "Control frame is transmitted, the bit is cleared by the UMAC.   The "
     "following are the steps for generating the PAUSE Control frame: 1) Read "
     "and Verify this bit is cleared. If not cleared wait until the bit is "
     "cleared (polling) 2) Program the Pause Frame SA and PAUSE_TIME registers "
     "3) Set this bit. 4) Poll this bit until the bit is cleared."},

};
reg_decoder_t mcmac3_fcfrmgen_flds = {1, mcmac3_fcfrmgen_fld_list, 16};

reg_decoder_fld_t mcmac3_fcdaddrlo_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     11028,
     "This byte contains the first Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac3_fcdaddrlo_flds = {1, mcmac3_fcdaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac3_fcdaddrmid_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     11029,
     "This byte contains the third Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac3_fcdaddrmid_flds = {1, mcmac3_fcdaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac3_fcdaddrhi_fld_list[] = {
    {"fcdaddr",
     15,
     0,
     0,
     11030,
     "This byte contains the fifth Byte of the Destination Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac3_fcdaddrhi_flds = {1, mcmac3_fcdaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac3_fcsaddrlo_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     11031,
     "This byte contains the first Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac3_fcsaddrlo_flds = {1, mcmac3_fcsaddrlo_fld_list, 16};

reg_decoder_fld_t mcmac3_fcsaddrmid_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     11032,
     "This byte contains the third Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac3_fcsaddrmid_flds = {1, mcmac3_fcsaddrmid_fld_list, 16};

reg_decoder_fld_t mcmac3_fcsaddrhi_fld_list[] = {
    {"fcsaddr",
     15,
     0,
     0,
     11033,
     "This byte contains the fifth Byte of the Source Address field as it "
     "appears in the transmitting PAUSE Flow Control Frame or Priority PAUSE "
     "Flow Control Frames on this Channel."},

};
reg_decoder_t mcmac3_fcsaddrhi_flds = {1, mcmac3_fcsaddrhi_fld_list, 16};

reg_decoder_fld_t mcmac3_fcpausetime_fld_list[] = {
    {"fcpausetime",
     15,
     0,
     0,
     11034,
     "This contains the Pause-Time value that is used in the generated PAUSE "
     "Control Frame under software control for this Channel. The 16-bit value "
     "is sent out as defined in the IEEE 802.3 Specification with most "
     "significant byte first followed by least significant byte."},

};
reg_decoder_t mcmac3_fcpausetime_flds = {1, mcmac3_fcpausetime_fld_list, 16};

reg_decoder_fld_t mcmac3_xoffpausetime_fld_list[] = {
    {"xoffpausetime",
     15,
     0,
     0,
     11035,
     "This field contains the Pause-Time value that is used in the generated "
     "PAUSE Control Frame when the XOFF PAUSE Frame is to be transmitted. This "
     "field is used both in the regular PAUSE flow Control Frames and Priority "
     "PAUSE Flow Control Frames. The 16-bit value is sent out as defined in "
     "the IEEE 802.3 Specification with most significant byte first followed "
     "by least significant byte"},

};
reg_decoder_t mcmac3_xoffpausetime_flds = {
    1, mcmac3_xoffpausetime_fld_list, 16};

reg_decoder_fld_t mcmac3_xonpausetime_fld_list[] = {
    {"xonpausetime",
     15,
     0,
     0,
     11036,
     "This field contains the Pause-Time value that is used in the generated "
     "PAUSE Control Frame when the XON PAUSE Frame is to be transmitted. This "
     "field is used both in the regular PAUSE flow Control Frames and Priority "
     "PAUSE Flow Control Frames. The 16-bit value is sent out as defined in "
     "the IEEE 802.3 Specification with most significant byte first followed "
     "by least significant byte"},

};
reg_decoder_t mcmac3_xonpausetime_flds = {1, mcmac3_xonpausetime_fld_list, 16};

reg_decoder_fld_t mcmac3_txtsinfo_fld_list[] = {
    {"tsid",
     1,
     0,
     0,
     11037,
     "This field contains the Transmit Timestamp ID associated with the Time "
     "Stamp that is present in the TimeStamp Value Register."},

    {"tsvld",
     15,
     15,
     0,
     11038,
     "This bit indicates whether the Transmit Timestamp Value Register has "
     "valid Value or not. This bit is set when the Timestamp Value register "
     "has been loaded with a new Timestamp Value from the TimeStamp FIFO."},

};
reg_decoder_t mcmac3_txtsinfo_flds = {2, mcmac3_txtsinfo_fld_list, 16};

reg_decoder_fld_t mcmac3_tsv0_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     11039,
     "This field contains the bits [15:0] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac3_tsv0_flds = {1, mcmac3_tsv0_fld_list, 16};

reg_decoder_fld_t mcmac3_tsv1_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     11040,
     "This field contains the bits [31:16] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac3_tsv1_flds = {1, mcmac3_tsv1_fld_list, 16};

reg_decoder_fld_t mcmac3_tsv2_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     11041,
     "This field contains the bits [47:32] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac3_tsv2_flds = {1, mcmac3_tsv2_fld_list, 16};

reg_decoder_fld_t mcmac3_tsv3_fld_list[] = {
    {"ts",
     15,
     0,
     0,
     11042,
     "This field contains the bits [63:48] of the TimeStamp Value associated "
     "with the TimeStamp ID from the Timestamp Info Register.  When there is "
     "no internally latched Timestamp, then this field returns 0xFFFF"},

};
reg_decoder_t mcmac3_tsv3_flds = {1, mcmac3_tsv3_fld_list, 16};

reg_decoder_fld_t mcmac3_txtsdelta_fld_list[] = {
    {"txtsdelta",
     15,
     0,
     0,
     11043,
     "This is the delta value that is subtracted from the latched Tx Timestamp "
     "Value before it is passed along with the Frame SOF."},

};
reg_decoder_t mcmac3_txtsdelta_flds = {1, mcmac3_txtsdelta_fld_list, 16};

reg_decoder_fld_t mcmac3_rxtsdelta_fld_list[] = {
    {"rxtsdelta",
     15,
     0,
     0,
     11044,
     "This is the delta value that is subtracted from the latched Rx Timestamp "
     "Value before it is passed along with the Frame SOF."},

};
reg_decoder_t mcmac3_rxtsdelta_flds = {1, mcmac3_rxtsdelta_fld_list, 16};

reg_decoder_fld_t mcmac3_minframesize_fld_list[] = {
    {"minframesize",
     7,
     0,
     0,
     11045,
     "This field is used to determine the MinFrameSize in Transmit and Receive "
     "operation.  The MinFrameSize should be programmed with a value which is "
     "multiple of 4 and with a minimum value of 32."},

};
reg_decoder_t mcmac3_minframesize_flds = {1, mcmac3_minframesize_fld_list, 16};

reg_decoder_fld_t mcmac3_txvlantag_fld_list[] = {
    {"txvlantag",
     15,
     0,
     0,
     11046,
     "This defines the VLAN Tag that is used for comparing and identifying "
     "VLAN frames on Transmit Side. "},

};
reg_decoder_t mcmac3_txvlantag_flds = {1, mcmac3_txvlantag_fld_list, 16};

reg_decoder_fld_t mcmac3_fint0_fld_list[] = {
    {"ovrdint10", 1, 0, 0, 11047, "TX underrun Interrupt override"},

};
reg_decoder_t mcmac3_fint0_flds = {1, mcmac3_fint0_fld_list, 16};

reg_decoder_fld_t mcmac3_fint1_fld_list[] = {
    {"ovrdint32", 1, 0, 0, 11048, "TX timestamp fifo overflow override"},

};
reg_decoder_t mcmac3_fint1_flds = {1, mcmac3_fint1_fld_list, 16};

reg_decoder_fld_t mcmac3_fint2_fld_list[] = {
    {"ovrdint654", 2, 0, 0, 11049, "RX Local Fault Interrupt override"},

};
reg_decoder_t mcmac3_fint2_flds = {1, mcmac3_fint2_fld_list, 16};

reg_decoder_fld_t mcmac3_slotclkcnt_fld_list[] = {
    {"slotclkcnt",
     7,
     0,
     0,
     11050,
     "When this field is set to zero, the transmit and receive MAC uses "
     "internal DataValid/DataReady to count the valid clocks for the Slot "
     "Timer Logic. In this mode the Slot is 512 Bit times based on the line "
     "rate.  When this field is non-zero, this field determines the number of "
     "Clocks (Core_clk_i) for the Slot Timer.   The Slot Timer is used to "
     "decrement the PAUSE Quanta."},

};
reg_decoder_t mcmac3_slotclkcnt_flds = {1, mcmac3_slotclkcnt_fld_list, 16};

reg_decoder_fld_t mcmac3_txdebug_fld_list[] = {
    {"stompeoperr",
     0,
     0,
     0,
     11051,
     "When set the EOP/ERR is ignored by the MAC."},

    {"txlfault",
     1,
     1,
     0,
     11052,
     "When set, the MAC continuously transmits L_FAULT ordered set."},

    {"txrfault",
     2,
     2,
     0,
     11053,
     "When set, the MAC continuously transmits R_FAULT ordered set."},

    {"txidle",
     3,
     3,
     0,
     11054,
     "When set, the MAC continuously transmits IDLE Pattern."},

    {"txtestpattern",
     4,
     4,
     0,
     11055,
     "This should be set when the PCS is programmed to transmit Test Pattern. "
     "This bit does not have any functionality in the MAC other than in the "
     "generation of TX_GOOD signal output. "},

    {"forcerxxoff",
     5,
     5,
     0,
     11056,
     "When set, the XOFF (for all priorities) is forced on APP_FIFO Interface "
     "when a regular PAUSE frame is received and the Transmit MAC is not "
     "disabled. When clear, normal PAUSE frame reception behavior of disabling "
     "TxMAC"},

    {"txautodraintxdisable",
     6,
     6,
     0,
     11057,
     "TxAutoDrainonTxDisable: When set, the TxAutoDrain function is enabled "
     "when the TXMAC is disabled (TXEnable is set to 1'b0). When clear, the "
     "frames are not drained when the MAC is disabled."},

    {"macdebug0",
     7,
     7,
     0,
     11058,
     "MAC Debug #0. This should be treated as reserved and not modified by "
     "User."},

    {"txvlantagena",
     8,
     8,
     0,
     11059,
     "This enables the VLAN Tag comparison and identification of VLAN frames "
     "on Transmit Side"},

    {"ovrdtxdisonpause",
     9,
     9,
     0,
     11060,
     "When set, the TxDisable on RxPause function is overridden. The Tx Is not "
     "disabled, but the signal ff_Rxfcxoff signal is generated on the APP_FIFO "
     "Interface. When cleared, the normal Pause function is enabled where the "
     "TXMAC is disabled for the duration of PAUSE Time."},

};
reg_decoder_t mcmac3_txdebug_flds = {10, mcmac3_txdebug_fld_list, 16};

reg_decoder_fld_t fifoctrl0_ctrl1_fld_list[] = {
    {"txfullthres4ch",
     3,
     0,
     0,
     11061,
     "FIFO Almost Full Threshold in 4 Channel Mode This determines the maximum "
     "number of Additional Valid Strobes that the Application can generate "
     "(FIFO DEPTH - Threshold Value) after the FIFO Almost Full is asserted "
     "(ff_txafullN_o) in FIFO based protocol. This is assuming that the "
     "Minimum Sized frame on the Application Interface is 17Bytes or more (SOF "
     "and EOF not on the same clock) The range of legal values are 2 to 12."},

    {"txfullthres2ch",
     8,
     4,
     0,
     11062,
     "FIFO Almost Full Threshold in 2 Channel Mode This determines the maximum "
     "number of Additional Valid Strobes that the Application can generate "
     "(FIFO DEPTH - Threshold Value) after the FIFO Almost Full is asserted "
     "(ff_txafullN_o) in FIFO based protocol. This is assuming that the "
     "Minimum Sized frame on the Application Interface is 17Bytes or more (SOF "
     "and EOF not on the same clock) The range of legal values are 2 to 28."},

    {"txfullthres1ch",
     14,
     9,
     0,
     11063,
     "FIFO Almost Full Threshold in 1 Channel Mode This determines the maximum "
     "number of Additional Valid Strobes that the Application can generate "
     "(FIFO DEPTH - Threshold Value) after the FIFO Almost Full is asserted "
     "(ff_txafullN_o) in FIFO based protocol. This is assuming that the "
     "Minimum Sized frame on the Application Interface is 17Bytes or more (SOF "
     "and EOF not on the same clock) The range of legal values are 2 to 60."},

    {"s2chmapena",
     15,
     15,
     0,
     11064,
     "When this bit is set, the FIFO_ARB generates the Channel Cycling based "
     "on the programmable mapping. When this is reset, the Channel cycling is "
     "based on the Global Mode and Global Mode#2 values. This Channel Cycling "
     "is used to read write APP_FIFO in TX and RX direction respectively "
     "(between the APP_FIFO and MC_MAC Interface)."},

};
reg_decoder_t fifoctrl0_ctrl1_flds = {4, fifoctrl0_ctrl1_fld_list, 16};

reg_decoder_fld_t fifoctrl0_ctrl2_fld_list[] = {
    {"usepingpongbuff",
     2,
     2,
     0,
     11065,
     "When this bit is set, it is used to implement the Ping-pong buffer "
     "scheme in programming the Slot to Channel Mapping in the TxArb Module. "
     "When reset, the programmed Slot to Channel Mapping values are used "
     "directly."},

    {"updatechmapping",
     3,
     3,
     0,
     11066,
     "Write to this register with a value of all 1's confirms the new Slot to "
     "Channel Mapping values. The Hardware will load the programmed Slot to "
     "Channel Values into its internal Registers upon write to this register. "
     "This is a write only register from software and read from this register "
     "returns all zeros."},

};
reg_decoder_t fifoctrl0_ctrl2_flds = {2, fifoctrl0_ctrl2_fld_list, 16};

reg_decoder_fld_t fifoctrl0_appfifolpbk_fld_list[] = {
    {"ench0",
     0,
     0,
     0,
     11067,
     "Setting this bit enables the Loopback on the MAC's FIFO Interface for "
     "Channel#0. The Receive frames are looped back on to the transmit MAC "
     "module for this Channel#N.  When this loopback is enabled, the data "
     "received from the Application/Application logic is ignored for "
     "Channel#o."},

    {"ench1",
     1,
     1,
     0,
     11068,
     "Setting this bit enables the Loopback on the MAC's FIFO Interface for "
     "Channel#1. The Receive frames are looped back on to the transmit MAC "
     "module for this Channel#N.  When this loopback is enabled, the data "
     "received from the Application/Application logic is ignored for "
     "Channel#1."},

    {"ench2",
     2,
     2,
     0,
     11069,
     "Setting this bit enables the Loopback on the MAC's FIFO Interface for "
     "Channel#2. The Receive frames are looped back on to the transmit MAC "
     "module for this Channel#N.  When this loopback is enabled, the data "
     "received from the Application/Application logic is ignored for "
     "Channel#2."},

    {"ench3",
     3,
     3,
     0,
     11070,
     "Setting this bit enables the Loopback on the MAC's FIFO Interface for "
     "Channel#3. The Receive frames are looped back on to the transmit MAC "
     "module for this Channel#N.  When this loopback is enabled, the data "
     "received from the Application/Application logic is ignored for "
     "Channel#3."},

};
reg_decoder_t fifoctrl0_appfifolpbk_flds = {
    4, fifoctrl0_appfifolpbk_fld_list, 16};

reg_decoder_fld_t fifoctrl0_appfifolpthrsh_fld_list[] = {
    {"appfifolpthrsh",
     5,
     0,
     0,
     11071,
     "In the FIFO Loopback mode, this determines the number of entries to be "
     "present in the Transmit FIFO before the frames are being transmitted "
     "out. This is to avoid Underun on Transmit as the Receive stream might "
     "have Alignment patterns, etc."},

};
reg_decoder_t fifoctrl0_appfifolpthrsh_flds = {
    1, fifoctrl0_appfifolpthrsh_fld_list, 16};

reg_decoder_fld_t fifoctrl0_appfifoportmap0_fld_list[] = {
    {"portmap0",
     2,
     0,
     0,
     11072,
     "The value defines the port# which is mapped to Ch#0."},

    {"portmap1",
     5,
     3,
     0,
     11073,
     "The value defines the port# which is mapped to Ch#1."},

    {"portmap2",
     8,
     6,
     0,
     11074,
     "The value defines the port# which is mapped to Ch#2."},

    {"portmap3",
     11,
     9,
     0,
     11075,
     "The value defines the port# which is mapped to Ch#3."},

    {"portmap0ena", 12, 12, 0, 11076, "Enable APP FIFO Port Map #0 field."},

    {"portmap1ena", 13, 13, 0, 11077, "Enable APP FIFO Port Map #1 field."},

    {"portmap2ena", 14, 14, 0, 11078, "Enable APP FIFO Port Map #2 field."},

    {"portmap3ena", 15, 15, 0, 11079, "Enable APP FIFO Port Map #3 field."},

};
reg_decoder_t fifoctrl0_appfifoportmap0_flds = {
    8, fifoctrl0_appfifoportmap0_fld_list, 16};

reg_decoder_fld_t fifoctrl0_rxfifoctrl0_fld_list[] = {
    {"sofmingap0",
     4,
     0,
     0,
     11080,
     "Min Gap between Consecutive SOFs on Same Channel, in presence of Short "
     "Packets"},

    {"enminframepadding0",
     5,
     5,
     0,
     11081,
     "Enable Rx Frame Padding to MinFrame Size"},

    {"minframepadding0",
     13,
     6,
     0,
     11082,
     "Min Frame Size for Padding (Multiple of 16 Bytes)"},

};
reg_decoder_t fifoctrl0_rxfifoctrl0_flds = {
    3, fifoctrl0_rxfifoctrl0_fld_list, 16};

reg_decoder_fld_t fifoctrl0_rxfifoctrl1_fld_list[] = {
    {"sofmingap1",
     4,
     0,
     0,
     11083,
     "Min Gap between Consecutive SOFs on Same Channel, in presence of Short "
     "Packets"},

    {"enminframepadding1",
     5,
     5,
     0,
     11084,
     "Enable Rx Frame Padding to MinFrame Size"},

    {"minframepadding1",
     13,
     6,
     0,
     11085,
     "Min Frame Size for Padding (Multiple of 16 Bytes)"},

};
reg_decoder_t fifoctrl0_rxfifoctrl1_flds = {
    3, fifoctrl0_rxfifoctrl1_fld_list, 16};

reg_decoder_fld_t fifoctrl0_rxfifoctrl2_fld_list[] = {
    {"sofmingap2",
     4,
     0,
     0,
     11086,
     "Min Gap between Consecutive SOFs on Same Channel, in presence of Short "
     "Packets"},

    {"enminframepadding2",
     5,
     5,
     0,
     11087,
     "Enable Rx Frame Padding to MinFrame Size"},

    {"minframepadding2",
     13,
     6,
     0,
     11088,
     "Min Frame Size for Padding (Multiple of 16 Bytes)"},

};
reg_decoder_t fifoctrl0_rxfifoctrl2_flds = {
    3, fifoctrl0_rxfifoctrl2_fld_list, 16};

reg_decoder_fld_t fifoctrl0_rxfifoctrl3_fld_list[] = {
    {"sofmingap3",
     4,
     0,
     0,
     11089,
     "Min Gap between Consecutive SOFs on Same Channel, in presence of Short "
     "Packets"},

    {"enminframepadding3",
     5,
     5,
     0,
     11090,
     "Enable Rx Frame Padding to MinFrame Size"},

    {"minframepadding3",
     13,
     6,
     0,
     11091,
     "Min Frame Size for Padding (Multiple of 16 Bytes)"},

};
reg_decoder_t fifoctrl0_rxfifoctrl3_flds = {
    3, fifoctrl0_rxfifoctrl3_fld_list, 16};

reg_decoder_fld_t fifoctrl0_txfifoctrl0_fld_list[] = {
    {"txthreshold0",
     5,
     0,
     0,
     11092,
     "The field indicates the threshold which is used by APP_FIFO before "
     "reading out the data from TX_FIFO and sending it to MAC. When the FIFO "
     "Fill level is equal to or greater than the Threshold value or EOF is "
     "seen, then the FIFO is read out and sent to MAC.  To Avoid deadlock, "
     "this value should never be greater than the "
     "(FIFO_DEPTH-SEL_AFULLTHRESH-N_ADDITIONAL_TXVALIDS).  Here, FIFO_DEPTH = "
     "Depth of the FIFO associated with Ch#0, SEL_AFULLTHRES = Selected Almost "
     "Full Threshold for Ch#0 and N_ADDITIONAL_TXVALIDS = Number of Additional "
     "Valids that APP Logic generates after the assertion of TxAfull on Ch#0."},

};
reg_decoder_t fifoctrl0_txfifoctrl0_flds = {
    1, fifoctrl0_txfifoctrl0_fld_list, 16};

reg_decoder_fld_t fifoctrl0_txfifoctrl1_fld_list[] = {
    {"txthreshold1",
     5,
     0,
     0,
     11093,
     "The field indicates the threshold which is used by APP_FIFO before "
     "reading out the data from TX_FIFO and sending it to MAC. When the FIFO "
     "Fill level is equal to or greater than the Threshold value or EOF is "
     "seen, then the FIFO is read out and sent to MAC. To Avoid deadlock, this "
     "value should never be greater than the "
     "(FIFO_DEPTH-SEL_AFULLTHRESH-N_ADDITIONAL_TXVALIDS).  Here, FIFO_DEPTH = "
     "Depth of the FIFO associated with Ch#0, SEL_AFULLTHRES = Selected Almost "
     "Full Threshold for Ch#0 and N_ADDITIONAL_TXVALIDS = Number of Additional "
     "Valids that APP Logic generates after the assertion of TxAfull on Ch#1."},

};
reg_decoder_t fifoctrl0_txfifoctrl1_flds = {
    1, fifoctrl0_txfifoctrl1_fld_list, 16};

reg_decoder_fld_t fifoctrl0_txfifoctrl2_fld_list[] = {
    {"txthreshold2",
     5,
     0,
     0,
     11094,
     "The field indicates the threshold which is used by APP_FIFO before "
     "reading out the data from TX_FIFO and sending it to MAC. When the FIFO "
     "Fill level is equal to or greater than the Threshold value or EOF is "
     "seen, then the FIFO is read out and sent to MAC. To Avoid deadlock, this "
     "value should never be greater than the "
     "(FIFO_DEPTH-SEL_AFULLTHRESH-N_ADDITIONAL_TXVALIDS).  Here, FIFO_DEPTH = "
     "Depth of the FIFO associated with Ch#0, SEL_AFULLTHRES = Selected Almost "
     "Full Threshold for Ch#0 and N_ADDITIONAL_TXVALIDS = Number of Additional "
     "Valids that APP Logic generates after the assertion of TxAfull on Ch#2."},

};
reg_decoder_t fifoctrl0_txfifoctrl2_flds = {
    1, fifoctrl0_txfifoctrl2_fld_list, 16};

reg_decoder_fld_t fifoctrl0_txfifoctrl3_fld_list[] = {
    {"txthreshold3",
     5,
     0,
     0,
     11095,
     "The field indicates the threshold which is used by APP_FIFO before "
     "reading out the data from TX_FIFO and sending it to MAC. When the FIFO "
     "Fill level is equal to or greater than the Threshold value or EOF is "
     "seen, then the FIFO is read out and sent to MAC. To Avoid deadlock, this "
     "value should never be greater than the "
     "(FIFO_DEPTH-SEL_AFULLTHRESH-N_ADDITIONAL_TXVALIDS).  Here, FIFO_DEPTH = "
     "Depth of the FIFO associated with Ch#0, SEL_AFULLTHRES = Selected Almost "
     "Full Threshold for Ch#0 and N_ADDITIONAL_TXVALIDS = Number of Additional "
     "Valids that APP Logic generates after the assertion of TxAfull on Ch#3."},

};
reg_decoder_t fifoctrl0_txfifoctrl3_flds = {
    1, fifoctrl0_txfifoctrl3_fld_list, 16};

reg_decoder_fld_t fifoctrl0_chmap0_fld_list[] = {
    {"slot0chmap", 2, 0, 0, 11096, "Channel Number to be used for Slot#0"},

    {"slot1chmap", 5, 3, 0, 11097, "Channel Number to be used for Slot#1"},

    {"slot2chmap", 8, 6, 0, 11098, "Channel Number to be used for Slot#2"},

    {"slot3chmap", 11, 9, 0, 11099, "Channel Number to be used for Slot#3"},

};
reg_decoder_t fifoctrl0_chmap0_flds = {4, fifoctrl0_chmap0_fld_list, 16};

reg_decoder_fld_t fifoctrl0_chmap1_fld_list[] = {
    {"slot4chmap", 2, 0, 0, 11100, "Channel Number to be used for Slot#4"},

    {"slot5chmap", 5, 3, 0, 11101, "Channel Number to be used for Slot#5"},

    {"slot6chmap", 8, 6, 0, 11102, "Channel Number to be used for Slot#6"},

    {"slot7chmap", 11, 9, 0, 11103, "Channel Number to be used for Slot#7"},

};
reg_decoder_t fifoctrl0_chmap1_flds = {4, fifoctrl0_chmap1_fld_list, 16};

reg_decoder_fld_t fifoctrl0_parityctrl_fld_list[] = {
    {"rxparityen",
     0,
     0,
     0,
     11104,
     "This bit when set, enables the parity Protection on the RX APP_FIFO. "
     "When cleared, the Parity check is not performed."},

    {"txparityen",
     1,
     1,
     0,
     11105,
     "This bit when set, enables the parity Protection on the TX APP_FIFO. "
     "When cleared, the Parity check is not performed."},

};
reg_decoder_t fifoctrl0_parityctrl_flds = {
    2, fifoctrl0_parityctrl_fld_list, 16};

reg_decoder_fld_t fifoctrl0_fint0_fld_list[] = {
    {"ovrd0int0",
     0,
     0,
     0,
     11106,
     "Fifoctrl Ch#0 RX Fifo Error Interrupt Override"},

    {"ovrd0int1",
     1,
     1,
     0,
     11107,
     "Fifoctrl Ch#0 TX Fifo Overflow Error Interrupt Override"},

    {"ovrd0int2",
     2,
     2,
     0,
     11108,
     "Fifoctrl Ch#0 TX Fifo DCNT Protocol Violation Error Interrupt Override"},

    {"ovrd0int3",
     3,
     3,
     0,
     11109,
     "Fifoctrl Ch#0 TX Fifo SOF/EOF Protocol Violation Error Interrupt "
     "Override"},

    {"ovrd0int4",
     4,
     4,
     0,
     11110,
     "Fifoctrl Ch#1 RX Fifo Error Interrupt Override"},

    {"ovrd0int5",
     5,
     5,
     0,
     11111,
     "Fifoctrl Ch#1 TX Fifo Overflow Error Interrupt Override"},

    {"ovrd0int6",
     6,
     6,
     0,
     11112,
     "Fifoctrl Ch#1 TX Fifo DCNT Protocol Violation Error Interrupt Override"},

    {"ovrd0int7",
     7,
     7,
     0,
     11113,
     "Fifoctrl Ch#1 TX Fifo SOF/EOF Protocol Violation Error Interrupt "
     "Override"},

    {"ovrd0int8",
     8,
     8,
     0,
     11114,
     "Fifoctrl Ch#2 RX Fifo Error Interrupt Override"},

    {"ovrd0int9",
     9,
     9,
     0,
     11115,
     "Fifoctrl Ch#2 TX Fifo Overflow Error Interrupt Override"},

    {"ovrd0int10",
     10,
     10,
     0,
     11116,
     "Fifoctrl Ch#2 TX Fifo DCNT Protocol Violation Error Interrupt Override"},

    {"ovrd0int11",
     11,
     11,
     0,
     11117,
     "Fifoctrl Ch#2 TX Fifo SOF/EOF Protocol Violation Error Interrupt "
     "Override"},

    {"ovrd0int12",
     12,
     12,
     0,
     11118,
     "Fifoctrl Ch#3 RX Fifo Error Interrupt Override"},

    {"ovrd0int13",
     13,
     13,
     0,
     11119,
     "Fifoctrl Ch#3 TX Fifo Overflow Error Interrupt Override"},

    {"ovrd0int14",
     14,
     14,
     0,
     11120,
     "Fifoctrl Ch#3 TX Fifo DCNT Protocol Violation Error Interrupt Override"},

    {"ovrd0int15",
     15,
     15,
     0,
     11121,
     "Fifoctrl Ch#3 TX Fifo SOF/EOF Protocol Violation Error Interrupt "
     "Override"},

};
reg_decoder_t fifoctrl0_fint0_flds = {16, fifoctrl0_fint0_fld_list, 16};

reg_decoder_fld_t fifoctrl0_fint2_fld_list[] = {
    {"ovrd2int0", 0, 0, 0, 11122, "TxFifo Parity Error"},

    {"ovrd2int1", 1, 1, 0, 11123, "RxFifo Parity Error"},

};
reg_decoder_t fifoctrl0_fint2_flds = {2, fifoctrl0_fint2_fld_list, 16};

reg_decoder_fld_t fifoctrl0_spare1_fld_list[] = {
    {"spare1", 7, 0, 0, 11124, "Spare1"},

};
reg_decoder_t fifoctrl0_spare1_flds = {1, fifoctrl0_spare1_fld_list, 16};

reg_decoder_fld_t fifoctrl0_spare0_fld_list[] = {
    {"spare0", 7, 0, 0, 11125, "Spare0"},

};
reg_decoder_t fifoctrl0_spare0_flds = {1, fifoctrl0_spare0_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_ctrl1_fld_list[] = {
    {"reset",
     15,
     15,
     0,
     11126,
     "Holds the PCS/FEC Channel in software reset    1'b1 : Reset    1'b0 : "
     "Normal Operation"},

};
reg_decoder_t hsmcpcs0_ctrl1_flds = {1, hsmcpcs0_ctrl1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_status1_fld_list[] = {
    {"fault", 7, 7, 0, 11127, "When asserted, a fault condition is detected."},

};
reg_decoder_t hsmcpcs0_status1_flds = {1, hsmcpcs0_status1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_mode_fld_list[] = {
    {"mlg",
     13,
     10,
     0,
     11128,
     "MLG Channel Mapping: 4'b1111 - 4'b1101 : Reserved 4'b1100:  40G slot 1 "
     "4'b1011 : 40G slot 0 4'b1010 : 10G slot 9 4'b1001: 10G slot 8 4'b1000: "
     "10G slot 7 4'b0111: 10G slot 6 4'b0110: 10G slot 5 4'b0101: 10G slot 4 "
     "4'b0100: 10G slot 3 4'b0011: 10G slot 2 4'b0010: 10G slot 1 4'b0001: 10G "
     "slot 0 4'b0000 : Disabled"},

    {"pma",
     5,
     2,
     0,
     11129,
     "4'b1111 : MLG  4'b1110 : Fiber Channel 4'b1101 - 4'b1000 : Reserved "
     "4'b0111 : BASE-R10 4'b0110 : BASE-R8 4'b0101 : BASE-R4 4'b0100 : BASE-R2 "
     "4'b0011 : BASE-R1 / BASE-X 4'b0010 : QSGMII 4'b0001 : SGMII 4'b0000 : "
     "Disabled"},

    {"fec",
     1,
     0,
     0,
     11130,
     "2'b11 : Reserved 2'b10 : RSFEC 2'b01 : FCFEC 2'b00 : None"},

};
reg_decoder_t hsmcpcs0_mode_flds = {3, hsmcpcs0_mode_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_mlgconfig_fld_list[] = {
    {"mlgconfig",
     1,
     0,
     0,
     11131,
     "2'b11 : 2x40G+2x10G 2'b10 : 4x10G+1x40G+2x10G 2'b01 : 1x40G+6x10G 2'b00 "
     ": 10x10G"},

    {"mlgena", 8, 8, 0, 11132, "Enables mlg datapath globally"},

    {"mlgswreset", 9, 9, 0, 11133, "Global mlg software reset"},

    {"mlgrsfecena", 10, 10, 0, 11134, "Enables RSFEC while mlg is active"},

    {"mlgaggscrena",
     11,
     11,
     0,
     11135,
     "When set to 1, scrambles 100G aggragate TX output and decrambles  RX "
     "input of the MLG logic."},

};
reg_decoder_t hsmcpcs0_mlgconfig_flds = {5, hsmcpcs0_mlgconfig_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_sts1_fld_list[] = {
    {"pcsstatus",
     12,
     12,
     0,
     11136,
     "When asserted PCS is in a fully operational state"},

    {"hiber", 1, 1, 0, 11137, "High Bit Error Rate detected"},

    {"blocklockall", 0, 0, 0, 11138, "PCS locked to received blocks"},

};
reg_decoder_t hsmcpcs0_sts1_flds = {3, hsmcpcs0_sts1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_sts2_fld_list[] = {
    {"bercount",
     13,
     8,
     0,
     11139,
     "Counts bad sync headers (lower order bits). Latches high order bercount "
     "bits on read. Resets internal counter."},

    {"erroredblocks",
     7,
     0,
     0,
     11140,
     "Counts errored blocks (lower order bits). Latches high order "
     "erroredblocks bits on read. Resets internal counter."},

};
reg_decoder_t hsmcpcs0_sts2_flds = {2, hsmcpcs0_sts2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatterna1_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11141, "Bits[15:0] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs0_testpatterna1_flds = {
    1, hsmcpcs0_testpatterna1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatterna2_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11142, "Bits[31:16] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs0_testpatterna2_flds = {
    1, hsmcpcs0_testpatterna2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatterna3_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11143, "Bits[47:32] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs0_testpatterna3_flds = {
    1, hsmcpcs0_testpatterna3_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatterna4_fld_list[] = {
    {"testpatterna", 9, 0, 0, 11144, "Bits[57:48] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs0_testpatterna4_flds = {
    1, hsmcpcs0_testpatterna4_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatternb1_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11145, "Bits[15:0] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs0_testpatternb1_flds = {
    1, hsmcpcs0_testpatternb1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatternb2_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11146, "Bits[31:16] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs0_testpatternb2_flds = {
    1, hsmcpcs0_testpatternb2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatternb3_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11147, "Bits[47:32] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs0_testpatternb3_flds = {
    1, hsmcpcs0_testpatternb3_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatternb4_fld_list[] = {
    {"testpatternb", 9, 0, 0, 11148, "Bits[57:48] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs0_testpatternb4_flds = {
    1, hsmcpcs0_testpatternb4_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatctrl_fld_list[] = {
    {"datapatternselect",
     0,
     0,
     0,
     11149,
     "Data input to pseudo random test mode. 0=Local Fault Data Pattern. "
     "1=Zeros Data Pattern"},

    {"testpatternselect",
     1,
     1,
     0,
     11150,
     "0=pseudo random test pattern. 1=square wave test pattern."},

    {"rtestmode", 2, 2, 0, 11151, "Enable receive test_pattern testing"},

    {"ttestmode", 3, 3, 0, 11152, "Enable transmit test_pattern"},

    {"scrambledidle",
     7,
     7,
     0,
     11153,
     "Enable scrambled idle test pattern mode"},

};
reg_decoder_t hsmcpcs0_testpatctrl_flds = {
    5, hsmcpcs0_testpatctrl_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_testpatternerrors_fld_list[] = {
    {"testpatternerrors",
     15,
     0,
     0,
     11154,
     "Counts test pattern reception errors"},

};
reg_decoder_t hsmcpcs0_testpatternerrors_flds = {
    1, hsmcpcs0_testpatternerrors_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_bercounthi_fld_list[] = {
    {"bercount",
     15,
     0,
     0,
     11155,
     "Counts bad sync headers (higher order bits). Cleared and latched when "
     "sts2 register is read."},

};
reg_decoder_t hsmcpcs0_bercounthi_flds = {1, hsmcpcs0_bercounthi_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_erroredblockshi_fld_list[] = {
    {"erroredblocks",
     13,
     0,
     0,
     11156,
     "Counts errored blocks (higher order bits). Cleared and latched when sts2 "
     "register is read."},

};
reg_decoder_t hsmcpcs0_erroredblockshi_flds = {
    1, hsmcpcs0_erroredblockshi_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_counters0_fld_list[] = {
    {"syncloss", 7, 0, 0, 11157, "Sync loss"},

    {"blocklockloss", 15, 8, 0, 11158, "Block lock loss"},

};
reg_decoder_t hsmcpcs0_counters0_flds = {2, hsmcpcs0_counters0_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_counters1_fld_list[] = {
    {"highber", 7, 0, 0, 11159, "High BER"},

    {"vlderr", 15, 8, 0, 11160, "Valid Error"},

};
reg_decoder_t hsmcpcs0_counters1_flds = {2, hsmcpcs0_counters1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_counters2_fld_list[] = {
    {"unkerr", 7, 0, 0, 11161, "Unknown Error"},

    {"invlderr", 15, 8, 0, 11162, "Invalid Error"},

};
reg_decoder_t hsmcpcs0_counters2_flds = {2, hsmcpcs0_counters2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_algnstat1_fld_list[] = {
    {"blocklock", 7, 0, 0, 11163, "Block lock on lane x"},

    {"alignstatus", 12, 12, 0, 11164, "All lanes are syncronized and aligned"},

};
reg_decoder_t hsmcpcs0_algnstat1_flds = {2, hsmcpcs0_algnstat1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_algnstat2_fld_list[] = {
    {"blocklock", 11, 0, 0, 11165, "Block lock on lane x"},

};
reg_decoder_t hsmcpcs0_algnstat2_flds = {1, hsmcpcs0_algnstat2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_algnstat3_fld_list[] = {
    {"amlock", 7, 0, 0, 11166, "Alignment marker lock on lane x"},

};
reg_decoder_t hsmcpcs0_algnstat3_flds = {1, hsmcpcs0_algnstat3_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_algnstat4_fld_list[] = {
    {"amlock", 11, 0, 0, 11167, "Alignment marker lock on lane x"},

};
reg_decoder_t hsmcpcs0_algnstat4_flds = {1, hsmcpcs0_algnstat4_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_amctrl_fld_list[] = {
    {"enable",
     0,
     0,
     0,
     11168,
     "Enables the use of custom alignment markers for 25G mode for IEEE draft "
     "compatibility "},

    {"amenable25g",
     1,
     1,
     0,
     11169,
     "Enables the insertion of alignment markers in 25G mode  for IEEE draft "
     "compatibility "},

};
reg_decoder_t hsmcpcs0_amctrl_flds = {2, hsmcpcs0_amctrl_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_am0low_fld_list[] = {
    {"am0", 15, 0, 0, 11170, "Programmable AM0 lower 16 bits"},

};
reg_decoder_t hsmcpcs0_am0low_flds = {1, hsmcpcs0_am0low_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_am0hi_fld_list[] = {
    {"am0", 15, 0, 0, 11171, "Programmable AM0 higher 16 bits"},

};
reg_decoder_t hsmcpcs0_am0hi_flds = {1, hsmcpcs0_am0hi_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_am1low_fld_list[] = {
    {"am1", 15, 0, 0, 11172, "Programmable AM1 lower 16 bits"},

};
reg_decoder_t hsmcpcs0_am1low_flds = {1, hsmcpcs0_am1low_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_am1hi_fld_list[] = {
    {"am1", 15, 0, 0, 11173, "Programmable AM1 higher 16 bits"},

};
reg_decoder_t hsmcpcs0_am1hi_flds = {1, hsmcpcs0_am1hi_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_am2low_fld_list[] = {
    {"am2", 15, 0, 0, 11174, "Programmable AM2 lower 16 bits"},

};
reg_decoder_t hsmcpcs0_am2low_flds = {1, hsmcpcs0_am2low_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_am2hi_fld_list[] = {
    {"am2", 15, 0, 0, 11175, "Programmable AM2 higher 16 bits"},

};
reg_decoder_t hsmcpcs0_am2hi_flds = {1, hsmcpcs0_am2hi_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_am3low_fld_list[] = {
    {"am3", 15, 0, 0, 11176, "Programmable AM3 lower 16 bits"},

};
reg_decoder_t hsmcpcs0_am3low_flds = {1, hsmcpcs0_am3low_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_am3hi_fld_list[] = {
    {"am3", 15, 0, 0, 11177, "Programmable AM3 higher 16 bits"},

};
reg_decoder_t hsmcpcs0_am3hi_flds = {1, hsmcpcs0_am3hi_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_fint0_fld_list[] = {
    {"ovrdint", 9, 0, 0, 11178, "block lock Interrupt override"},

};
reg_decoder_t hsmcpcs0_fint0_flds = {1, hsmcpcs0_fint0_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping0_fld_list[] = {
    {"lanemapping0",
     4,
     0,
     0,
     11179,
     "Indicates which PCS lane is received on lane 0"},

};
reg_decoder_t hsmcpcs0_lanemapping0_flds = {
    1, hsmcpcs0_lanemapping0_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping1_fld_list[] = {
    {"lanemapping1",
     4,
     0,
     0,
     11180,
     "Indicates which PCS lane is received on lane 1"},

};
reg_decoder_t hsmcpcs0_lanemapping1_flds = {
    1, hsmcpcs0_lanemapping1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping2_fld_list[] = {
    {"lanemapping2",
     4,
     0,
     0,
     11181,
     "Indicates which PCS lane is received on lane 2"},

};
reg_decoder_t hsmcpcs0_lanemapping2_flds = {
    1, hsmcpcs0_lanemapping2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping3_fld_list[] = {
    {"lanemapping3",
     4,
     0,
     0,
     11182,
     "Indicates which PCS lane is received on lane 3"},

};
reg_decoder_t hsmcpcs0_lanemapping3_flds = {
    1, hsmcpcs0_lanemapping3_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping4_fld_list[] = {
    {"lanemapping4",
     4,
     0,
     0,
     11183,
     "Indicates which PCS lane is received on lane 4"},

};
reg_decoder_t hsmcpcs0_lanemapping4_flds = {
    1, hsmcpcs0_lanemapping4_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping5_fld_list[] = {
    {"lanemapping5",
     4,
     0,
     0,
     11184,
     "Indicates which PCS lane is received on lane 5"},

};
reg_decoder_t hsmcpcs0_lanemapping5_flds = {
    1, hsmcpcs0_lanemapping5_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping6_fld_list[] = {
    {"lanemapping6",
     4,
     0,
     0,
     11185,
     "Indicates which PCS lane is received on lane 6"},

};
reg_decoder_t hsmcpcs0_lanemapping6_flds = {
    1, hsmcpcs0_lanemapping6_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping7_fld_list[] = {
    {"lanemapping7",
     4,
     0,
     0,
     11186,
     "Indicates which PCS lane is received on lane 7"},

};
reg_decoder_t hsmcpcs0_lanemapping7_flds = {
    1, hsmcpcs0_lanemapping7_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping8_fld_list[] = {
    {"lanemapping8",
     4,
     0,
     0,
     11187,
     "Indicates which PCS lane is received on lane 8"},

};
reg_decoder_t hsmcpcs0_lanemapping8_flds = {
    1, hsmcpcs0_lanemapping8_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping9_fld_list[] = {
    {"lanemapping9",
     4,
     0,
     0,
     11188,
     "Indicates which PCS lane is received on lane 9"},

};
reg_decoder_t hsmcpcs0_lanemapping9_flds = {
    1, hsmcpcs0_lanemapping9_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping10_fld_list[] = {
    {"lanemapping10",
     4,
     0,
     0,
     11189,
     "Indicates which PCS lane is received on lane 10"},

};
reg_decoder_t hsmcpcs0_lanemapping10_flds = {
    1, hsmcpcs0_lanemapping10_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping11_fld_list[] = {
    {"lanemapping11",
     4,
     0,
     0,
     11190,
     "Indicates which PCS lane is received on lane 11"},

};
reg_decoder_t hsmcpcs0_lanemapping11_flds = {
    1, hsmcpcs0_lanemapping11_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping12_fld_list[] = {
    {"lanemapping12",
     4,
     0,
     0,
     11191,
     "Indicates which PCS lane is received on lane 12"},

};
reg_decoder_t hsmcpcs0_lanemapping12_flds = {
    1, hsmcpcs0_lanemapping12_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping13_fld_list[] = {
    {"lanemapping13",
     4,
     0,
     0,
     11192,
     "Indicates which PCS lane is received on lane 13"},

};
reg_decoder_t hsmcpcs0_lanemapping13_flds = {
    1, hsmcpcs0_lanemapping13_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping14_fld_list[] = {
    {"lanemapping14",
     4,
     0,
     0,
     11193,
     "Indicates which PCS lane is received on lane 14"},

};
reg_decoder_t hsmcpcs0_lanemapping14_flds = {
    1, hsmcpcs0_lanemapping14_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping15_fld_list[] = {
    {"lanemapping15",
     4,
     0,
     0,
     11194,
     "Indicates which PCS lane is received on lane 15"},

};
reg_decoder_t hsmcpcs0_lanemapping15_flds = {
    1, hsmcpcs0_lanemapping15_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping16_fld_list[] = {
    {"lanemapping16",
     4,
     0,
     0,
     11195,
     "Indicates which PCS lane is received on lane 16"},

};
reg_decoder_t hsmcpcs0_lanemapping16_flds = {
    1, hsmcpcs0_lanemapping16_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping17_fld_list[] = {
    {"lanemapping17",
     4,
     0,
     0,
     11196,
     "Indicates which PCS lane is received on lane 17"},

};
reg_decoder_t hsmcpcs0_lanemapping17_flds = {
    1, hsmcpcs0_lanemapping17_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping18_fld_list[] = {
    {"lanemapping18",
     4,
     0,
     0,
     11197,
     "Indicates which PCS lane is received on lane 18"},

};
reg_decoder_t hsmcpcs0_lanemapping18_flds = {
    1, hsmcpcs0_lanemapping18_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_lanemapping19_fld_list[] = {
    {"lanemapping19",
     4,
     0,
     0,
     11198,
     "Indicates which PCS lane is received on lane 19"},

};
reg_decoder_t hsmcpcs0_lanemapping19_flds = {
    1, hsmcpcs0_lanemapping19_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln0_fld_list[] = {
    {"biperrorsln0",
     15,
     0,
     0,
     11199,
     "Bit Interleaved Parity errors count on lane 0"},

};
reg_decoder_t hsmcpcs0_biperrorsln0_flds = {
    1, hsmcpcs0_biperrorsln0_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln1_fld_list[] = {
    {"biperrorsln1",
     15,
     0,
     0,
     11200,
     "Bit Interleaved Parity errors count on lane 1"},

};
reg_decoder_t hsmcpcs0_biperrorsln1_flds = {
    1, hsmcpcs0_biperrorsln1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln2_fld_list[] = {
    {"biperrorsln2",
     15,
     0,
     0,
     11201,
     "Bit Interleaved Parity errors count on lane 2"},

};
reg_decoder_t hsmcpcs0_biperrorsln2_flds = {
    1, hsmcpcs0_biperrorsln2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln3_fld_list[] = {
    {"biperrorsln3",
     15,
     0,
     0,
     11202,
     "Bit Interleaved Parity errors count on lane 3"},

};
reg_decoder_t hsmcpcs0_biperrorsln3_flds = {
    1, hsmcpcs0_biperrorsln3_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln4_fld_list[] = {
    {"biperrorsln4",
     15,
     0,
     0,
     11203,
     "Bit Interleaved Parity errors count on lane 4"},

};
reg_decoder_t hsmcpcs0_biperrorsln4_flds = {
    1, hsmcpcs0_biperrorsln4_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln5_fld_list[] = {
    {"biperrorsln5",
     15,
     0,
     0,
     11204,
     "Bit Interleaved Parity errors count on lane 5"},

};
reg_decoder_t hsmcpcs0_biperrorsln5_flds = {
    1, hsmcpcs0_biperrorsln5_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln6_fld_list[] = {
    {"biperrorsln6",
     15,
     0,
     0,
     11205,
     "Bit Interleaved Parity errors count on lane 6"},

};
reg_decoder_t hsmcpcs0_biperrorsln6_flds = {
    1, hsmcpcs0_biperrorsln6_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln7_fld_list[] = {
    {"biperrorsln7",
     15,
     0,
     0,
     11206,
     "Bit Interleaved Parity errors count on lane 7"},

};
reg_decoder_t hsmcpcs0_biperrorsln7_flds = {
    1, hsmcpcs0_biperrorsln7_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln8_fld_list[] = {
    {"biperrorsln8",
     15,
     0,
     0,
     11207,
     "Bit Interleaved Parity errors count on lane 8"},

};
reg_decoder_t hsmcpcs0_biperrorsln8_flds = {
    1, hsmcpcs0_biperrorsln8_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln9_fld_list[] = {
    {"biperrorsln9",
     15,
     0,
     0,
     11208,
     "Bit Interleaved Parity errors count on lane 9"},

};
reg_decoder_t hsmcpcs0_biperrorsln9_flds = {
    1, hsmcpcs0_biperrorsln9_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln10_fld_list[] = {
    {"biperrorsln10",
     15,
     0,
     0,
     11209,
     "Bit Interleaved Parity errors count on lane 10"},

};
reg_decoder_t hsmcpcs0_biperrorsln10_flds = {
    1, hsmcpcs0_biperrorsln10_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln11_fld_list[] = {
    {"biperrorsln11",
     15,
     0,
     0,
     11210,
     "Bit Interleaved Parity errors count on lane 11"},

};
reg_decoder_t hsmcpcs0_biperrorsln11_flds = {
    1, hsmcpcs0_biperrorsln11_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln12_fld_list[] = {
    {"biperrorsln12",
     15,
     0,
     0,
     11211,
     "Bit Interleaved Parity errors count on lane 12"},

};
reg_decoder_t hsmcpcs0_biperrorsln12_flds = {
    1, hsmcpcs0_biperrorsln12_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln13_fld_list[] = {
    {"biperrorsln13",
     15,
     0,
     0,
     11212,
     "Bit Interleaved Parity errors count on lane 13"},

};
reg_decoder_t hsmcpcs0_biperrorsln13_flds = {
    1, hsmcpcs0_biperrorsln13_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln14_fld_list[] = {
    {"biperrorsln14",
     15,
     0,
     0,
     11213,
     "Bit Interleaved Parity errors count on lane 14"},

};
reg_decoder_t hsmcpcs0_biperrorsln14_flds = {
    1, hsmcpcs0_biperrorsln14_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln15_fld_list[] = {
    {"biperrorsln15",
     15,
     0,
     0,
     11214,
     "Bit Interleaved Parity errors count on lane 15"},

};
reg_decoder_t hsmcpcs0_biperrorsln15_flds = {
    1, hsmcpcs0_biperrorsln15_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln16_fld_list[] = {
    {"biperrorsln16",
     15,
     0,
     0,
     11215,
     "Bit Interleaved Parity errors count on lane 16"},

};
reg_decoder_t hsmcpcs0_biperrorsln16_flds = {
    1, hsmcpcs0_biperrorsln16_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln17_fld_list[] = {
    {"biperrorsln17",
     15,
     0,
     0,
     11216,
     "Bit Interleaved Parity errors count on lane 17"},

};
reg_decoder_t hsmcpcs0_biperrorsln17_flds = {
    1, hsmcpcs0_biperrorsln17_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln18_fld_list[] = {
    {"biperrorsln18",
     15,
     0,
     0,
     11217,
     "Bit Interleaved Parity errors count on lane 18"},

};
reg_decoder_t hsmcpcs0_biperrorsln18_flds = {
    1, hsmcpcs0_biperrorsln18_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_biperrorsln19_fld_list[] = {
    {"biperrorsln19",
     15,
     0,
     0,
     11218,
     "Bit Interleaved Parity errors count on lane 19"},

};
reg_decoder_t hsmcpcs0_biperrorsln19_flds = {
    1, hsmcpcs0_biperrorsln19_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgbiterrorgap_fld_list[] = {
    {"dbgbiterrorgap",
     15,
     0,
     0,
     11219,
     "Distance in bits between inserted bit errors.  To use when bit error "
     "insertion is enabled.  A maximum of a single bit error may be sent per "
     "cycle."},

};
reg_decoder_t hsmcpcs0_dbgbiterrorgap_flds = {
    1, hsmcpcs0_dbgbiterrorgap_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgsyncheadgap_fld_list[] = {
    {"dbgsyncheadgap",
     15,
     0,
     0,
     11220,
     "Distance in words between corrupted sync headers.  To use when sync "
     "header corruption is enabled.  Headers are corrupted by setting sync[1] "
     "equal to sync[0]"},

};
reg_decoder_t hsmcpcs0_dbgsyncheadgap_flds = {
    1, hsmcpcs0_dbgsyncheadgap_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgcodegrp0_fld_list[] = {
    {"dbgcodegroup",
     1,
     0,
     0,
     11221,
     "Custom coded word to be inserted when insert code is raised.  Custom "
     "word will replace an existed word.  Bits [65:64]"},

};
reg_decoder_t hsmcpcs0_dbgcodegrp0_flds = {
    1, hsmcpcs0_dbgcodegrp0_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgcodegrp1_fld_list[] = {
    {"dbgcodegroup",
     15,
     0,
     0,
     11222,
     "Custom coded word to be inserted when insert code is raised.  Custom "
     "word will replace an existed word.  Bits [63:48]"},

};
reg_decoder_t hsmcpcs0_dbgcodegrp1_flds = {
    1, hsmcpcs0_dbgcodegrp1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgcodegrp2_fld_list[] = {
    {"dbgcodegroup",
     15,
     0,
     0,
     11223,
     "Custom coded word to be inserted when insert code is raised.  Custom "
     "word will replace an existed word.  Bits [47:32]"},

};
reg_decoder_t hsmcpcs0_dbgcodegrp2_flds = {
    1, hsmcpcs0_dbgcodegrp2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgcodegrp3_fld_list[] = {
    {"dbgcodegroup",
     15,
     0,
     0,
     11224,
     "Custom coded word to be inserted when insert code is raised.  Custom "
     "word will replace an existed word.  Bits [31:16]"},

};
reg_decoder_t hsmcpcs0_dbgcodegrp3_flds = {
    1, hsmcpcs0_dbgcodegrp3_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgcodegrp4_fld_list[] = {
    {"dbgcodegroup",
     15,
     0,
     0,
     11225,
     "Custom coded word to be inserted when insert code is raised.  Custom "
     "word will replace an existed word.  Bits [15:0]"},

};
reg_decoder_t hsmcpcs0_dbgcodegrp4_flds = {
    1, hsmcpcs0_dbgcodegrp4_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgctrl2_fld_list[] = {
    {"dbgbiterroren", 1, 1, 0, 11226, "Enable bit error injection"},

    {"dbgsyncheaderen", 2, 2, 0, 11227, "Enable sync header corruption"},

    {"dbginsertcode",
     5,
     5,
     0,
     11228,
     "Send a single custom code word when this bit is asserted"},

};
reg_decoder_t hsmcpcs0_dbgctrl2_flds = {3, hsmcpcs0_dbgctrl2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgbersmovrd_fld_list[] = {
    {"dbgdisablebermonitor",
     0,
     0,
     0,
     11229,
     "Set to 1 disables BER state machine by holding the INIT state"},

    {"dbgusecustombervals", 1, 1, 0, 11230, "Set to 1 to use custom xus timer"},

    {"dbgcustomxustimerval",
     8,
     2,
     0,
     11231,
     "Override value for bits[15:9] of BER state machine xus timer, all other "
     "xus timer bits are set to 0 when used. Enabled by dbgusecustombervals."},

    {"dbgcustombadmaxval",
     15,
     9,
     0,
     11232,
     "Override value for the number of bad sync headers required to cause the "
     "hiBER case. Enabled by dbgusecustombervals."},

};
reg_decoder_t hsmcpcs0_dbgbersmovrd_flds = {
    4, hsmcpcs0_dbgbersmovrd_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_spare0_fld_list[] = {
    {"spare0", 7, 0, 0, 11233, "Spare0"},

};
reg_decoder_t hsmcpcs0_spare0_flds = {1, hsmcpcs0_spare0_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgstat2_fld_list[] = {
    {"dbgdeskewoverflow",
     15,
     0,
     0,
     11234,
     "Indicates deskewfifos have overflowed."},

};
reg_decoder_t hsmcpcs0_dbgstat2_flds = {1, hsmcpcs0_dbgstat2_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgstat1_fld_list[] = {
    {"dbgtxgearboxfifoerr",
     3,
     0,
     0,
     11235,
     "Indicates gearboxes have overflowed or underflowed."},

    {"dbgdeskewoverflow",
     13,
     10,
     0,
     11236,
     "Indicates deskewfifos have overflowed. Clear on read of Debug Status 2 "
     "Register"},

};
reg_decoder_t hsmcpcs0_dbgstat1_flds = {2, hsmcpcs0_dbgstat1_fld_list, 16};

reg_decoder_fld_t hsmcpcs0_dbgctrl1_fld_list[] = {
    {"ena", 0, 0, 0, 11237, "Enable the PCS"},

    {"dbgbypassscrambler",
     1,
     1,
     0,
     11238,
     "When set to 1, bypasses scrambler and descrambler on all channels."},

    {"dbguseshorttimer",
     2,
     2,
     0,
     11239,
     "Send/Detect each alignment marker after 127 words per lane instead of "
     "16k"},

    {"dbgfullthreshold",
     9,
     6,
     0,
     11240,
     "Maximum number of entries that may be stored in the TX gearbox async "
     "FIFOs at one time. Allowable values 0-8."},

    {"decodetrapsel",
     12,
     10,
     0,
     11241,
     "TRAP specific decoded value. C_TYPE=3'h0; S_TYPE=3'h1; T_TYPE=3'h2; "
     "D_D_TYPE=3'h3; E_TYPE=3'h4; TRAP_REMOTE=3'h5; TRAP_LOCAL=3'h6; No "
     "trap=3'h7"},

    {"debuglowlatencymode",
     13,
     13,
     0,
     11242,
     "Assumes data to be error free in order to improve latency. VIOLATES "
     "IEEE."},

    {"fecenable",
     14,
     14,
     0,
     11243,
     "Use the FEC interfaces instead of normal data path (disable PCS status "
     "registers)"},

};
reg_decoder_t hsmcpcs0_dbgctrl1_flds = {7, hsmcpcs0_dbgctrl1_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_ctrl1_fld_list[] = {
    {"reset",
     15,
     15,
     0,
     11244,
     "Holds the PCS/FEC Channel in software reset    1'b1 : Reset    1'b0 : "
     "Normal Operation"},

};
reg_decoder_t hsmcpcs1_ctrl1_flds = {1, hsmcpcs1_ctrl1_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_status1_fld_list[] = {
    {"fault", 7, 7, 0, 11245, "When asserted, a fault condition is detected."},

};
reg_decoder_t hsmcpcs1_status1_flds = {1, hsmcpcs1_status1_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_mode_fld_list[] = {
    {"mlg",
     13,
     10,
     0,
     11246,
     "MLG Channel Mapping: 4'b1111 - 4'b1101 : Reserved 4'b1100:  40G slot 1 "
     "4'b1011 : 40G slot 0 4'b1010 : 10G slot 9 4'b1001: 10G slot 8 4'b1000: "
     "10G slot 7 4'b0111: 10G slot 6 4'b0110: 10G slot 5 4'b0101: 10G slot 4 "
     "4'b0100: 10G slot 3 4'b0011: 10G slot 2 4'b0010: 10G slot 1 4'b0001: 10G "
     "slot 0 4'b0000 : Disabled"},

    {"pma",
     5,
     2,
     0,
     11247,
     "4'b1111 : MLG  4'b1110 : Fiber Channel 4'b1101 - 4'b1000 : Reserved "
     "4'b0111 : BASE-R10 4'b0110 : BASE-R8 4'b0101 : BASE-R4 4'b0100 : BASE-R2 "
     "4'b0011 : BASE-R1 / BASE-X 4'b0010 : QSGMII 4'b0001 : SGMII 4'b0000 : "
     "Disabled"},

    {"fec",
     1,
     0,
     0,
     11248,
     "2'b11 : Reserved 2'b10 : RSFEC 2'b01 : FCFEC 2'b00 : None"},

};
reg_decoder_t hsmcpcs1_mode_flds = {3, hsmcpcs1_mode_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_sts1_fld_list[] = {
    {"pcsstatus",
     12,
     12,
     0,
     11249,
     "When asserted PCS is in a fully operational state"},

    {"hiber", 1, 1, 0, 11250, "High Bit Error Rate detected"},

    {"blocklockall", 0, 0, 0, 11251, "PCS locked to received blocks"},

};
reg_decoder_t hsmcpcs1_sts1_flds = {3, hsmcpcs1_sts1_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_sts2_fld_list[] = {
    {"bercount",
     13,
     8,
     0,
     11252,
     "Counts bad sync headers (lower order bits). Latches high order bercount "
     "bits on read. Resets internal counter."},

    {"erroredblocks",
     7,
     0,
     0,
     11253,
     "Counts errored blocks (lower order bits). Latches high order "
     "erroredblocks bits on read. Resets internal counter."},

};
reg_decoder_t hsmcpcs1_sts2_flds = {2, hsmcpcs1_sts2_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatterna1_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11254, "Bits[15:0] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs1_testpatterna1_flds = {
    1, hsmcpcs1_testpatterna1_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatterna2_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11255, "Bits[31:16] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs1_testpatterna2_flds = {
    1, hsmcpcs1_testpatterna2_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatterna3_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11256, "Bits[47:32] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs1_testpatterna3_flds = {
    1, hsmcpcs1_testpatterna3_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatterna4_fld_list[] = {
    {"testpatterna", 9, 0, 0, 11257, "Bits[57:48] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs1_testpatterna4_flds = {
    1, hsmcpcs1_testpatterna4_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatternb1_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11258, "Bits[15:0] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs1_testpatternb1_flds = {
    1, hsmcpcs1_testpatternb1_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatternb2_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11259, "Bits[31:16] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs1_testpatternb2_flds = {
    1, hsmcpcs1_testpatternb2_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatternb3_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11260, "Bits[47:32] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs1_testpatternb3_flds = {
    1, hsmcpcs1_testpatternb3_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatternb4_fld_list[] = {
    {"testpatternb", 9, 0, 0, 11261, "Bits[57:48] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs1_testpatternb4_flds = {
    1, hsmcpcs1_testpatternb4_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatctrl_fld_list[] = {
    {"datapatternselect",
     0,
     0,
     0,
     11262,
     "Data input to pseudo random test mode. 0=Local Fault Data Pattern. "
     "1=Zeros Data Pattern"},

    {"testpatternselect",
     1,
     1,
     0,
     11263,
     "0=pseudo random test pattern. 1=square wave test pattern."},

    {"rtestmode", 2, 2, 0, 11264, "Enable receive test_pattern testing"},

    {"ttestmode", 3, 3, 0, 11265, "Enable transmit test_pattern"},

    {"scrambledidle",
     7,
     7,
     0,
     11266,
     "Enable scrambled idle test pattern mode"},

};
reg_decoder_t hsmcpcs1_testpatctrl_flds = {
    5, hsmcpcs1_testpatctrl_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_testpatternerrors_fld_list[] = {
    {"testpatternerrors",
     15,
     0,
     0,
     11267,
     "Counts test pattern reception errors"},

};
reg_decoder_t hsmcpcs1_testpatternerrors_flds = {
    1, hsmcpcs1_testpatternerrors_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_bercounthi_fld_list[] = {
    {"bercount",
     15,
     0,
     0,
     11268,
     "Counts bad sync headers (higher order bits). Cleared and latched when "
     "sts2 register is read."},

};
reg_decoder_t hsmcpcs1_bercounthi_flds = {1, hsmcpcs1_bercounthi_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_erroredblockshi_fld_list[] = {
    {"erroredblocks",
     13,
     0,
     0,
     11269,
     "Counts errored blocks (higher order bits). Cleared and latched when sts2 "
     "register is read."},

};
reg_decoder_t hsmcpcs1_erroredblockshi_flds = {
    1, hsmcpcs1_erroredblockshi_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_counters0_fld_list[] = {
    {"syncloss", 7, 0, 0, 11270, "Sync loss"},

    {"blocklockloss", 15, 8, 0, 11271, "Block lock loss"},

};
reg_decoder_t hsmcpcs1_counters0_flds = {2, hsmcpcs1_counters0_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_counters1_fld_list[] = {
    {"highber", 7, 0, 0, 11272, "High BER"},

    {"vlderr", 15, 8, 0, 11273, "Valid Error"},

};
reg_decoder_t hsmcpcs1_counters1_flds = {2, hsmcpcs1_counters1_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_counters2_fld_list[] = {
    {"unkerr", 7, 0, 0, 11274, "Unknown Error"},

    {"invlderr", 15, 8, 0, 11275, "Invalid Error"},

};
reg_decoder_t hsmcpcs1_counters2_flds = {2, hsmcpcs1_counters2_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_algnstat1_fld_list[] = {
    {"blocklock", 0, 0, 0, 11276, "Block lock on lane x"},

};
reg_decoder_t hsmcpcs1_algnstat1_flds = {1, hsmcpcs1_algnstat1_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_amctrl_fld_list[] = {
    {"enable",
     0,
     0,
     0,
     11277,
     "Enables the use of custom alignment markers for 25G mode for IEEE draft "
     "compatibility "},

    {"amenable25g",
     1,
     1,
     0,
     11278,
     "Enables the insertion of alignment markers in 25G mode  for IEEE draft "
     "compatibility "},

};
reg_decoder_t hsmcpcs1_amctrl_flds = {2, hsmcpcs1_amctrl_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_fint0_fld_list[] = {
    {"ovrdint", 9, 0, 0, 11279, "block lock Interrupt override"},

};
reg_decoder_t hsmcpcs1_fint0_flds = {1, hsmcpcs1_fint0_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_dbgctrl2_fld_list[] = {
    {"dbgbiterroren", 1, 1, 0, 11280, "Enable bit error injection"},

    {"dbgsyncheaderen", 2, 2, 0, 11281, "Enable sync header corruption"},

    {"dbginsertcode",
     5,
     5,
     0,
     11282,
     "Send a single custom code word when this bit is asserted"},

};
reg_decoder_t hsmcpcs1_dbgctrl2_flds = {3, hsmcpcs1_dbgctrl2_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_dbgbersmovrd_fld_list[] = {
    {"dbgdisablebermonitor",
     0,
     0,
     0,
     11283,
     "Set to 1 disables BER state machine by holding the INIT state"},

    {"dbgusecustombervals", 1, 1, 0, 11284, "Set to 1 to use custom xus timer"},

    {"dbgcustomxustimerval",
     8,
     2,
     0,
     11285,
     "Override value for bits[15:9] of BER state machine xus timer, all other "
     "xus timer bits are set to 0 when used. Enabled by dbgusecustombervals."},

    {"dbgcustombadmaxval",
     15,
     9,
     0,
     11286,
     "Override value for the number of bad sync headers required to cause the "
     "hiBER case. Enabled by dbgusecustombervals."},

};
reg_decoder_t hsmcpcs1_dbgbersmovrd_flds = {
    4, hsmcpcs1_dbgbersmovrd_fld_list, 16};

reg_decoder_fld_t hsmcpcs1_dbgctrl1_fld_list[] = {
    {"dbgfullthreshold",
     9,
     6,
     0,
     11287,
     "Maximum number of entries that may be stored in the TX gearbox async "
     "FIFOs at one time. Allowable values 0-8."},

    {"debuglowlatencymode",
     13,
     13,
     0,
     11288,
     "Assumes data to be error free in order to improve latency. VIOLATES "
     "IEEE."},

};
reg_decoder_t hsmcpcs1_dbgctrl1_flds = {2, hsmcpcs1_dbgctrl1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_ctrl1_fld_list[] = {
    {"reset",
     15,
     15,
     0,
     11289,
     "Holds the PCS/FEC Channel in software reset    1'b1 : Reset    1'b0 : "
     "Normal Operation"},

};
reg_decoder_t hsmcpcs2_ctrl1_flds = {1, hsmcpcs2_ctrl1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_status1_fld_list[] = {
    {"fault", 7, 7, 0, 11290, "When asserted, a fault condition is detected."},

};
reg_decoder_t hsmcpcs2_status1_flds = {1, hsmcpcs2_status1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_mode_fld_list[] = {
    {"mlg",
     13,
     10,
     0,
     11291,
     "MLG Channel Mapping: 4'b1111 - 4'b1101 : Reserved 4'b1100:  40G slot 1 "
     "4'b1011 : 40G slot 0 4'b1010 : 10G slot 9 4'b1001: 10G slot 8 4'b1000: "
     "10G slot 7 4'b0111: 10G slot 6 4'b0110: 10G slot 5 4'b0101: 10G slot 4 "
     "4'b0100: 10G slot 3 4'b0011: 10G slot 2 4'b0010: 10G slot 1 4'b0001: 10G "
     "slot 0 4'b0000 : Disabled"},

    {"pma",
     5,
     2,
     0,
     11292,
     "4'b1111 : MLG  4'b1110 : Fiber Channel 4'b1101 - 4'b1000 : Reserved "
     "4'b0111 : BASE-R10 4'b0110 : BASE-R8 4'b0101 : BASE-R4 4'b0100 : BASE-R2 "
     "4'b0011 : BASE-R1 / BASE-X 4'b0010 : QSGMII 4'b0001 : SGMII 4'b0000 : "
     "Disabled"},

    {"fec",
     1,
     0,
     0,
     11293,
     "2'b11 : Reserved 2'b10 : RSFEC 2'b01 : FCFEC 2'b00 : None"},

};
reg_decoder_t hsmcpcs2_mode_flds = {3, hsmcpcs2_mode_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_sts1_fld_list[] = {
    {"pcsstatus",
     12,
     12,
     0,
     11294,
     "When asserted PCS is in a fully operational state"},

    {"hiber", 1, 1, 0, 11295, "High Bit Error Rate detected"},

    {"blocklockall", 0, 0, 0, 11296, "PCS locked to received blocks"},

};
reg_decoder_t hsmcpcs2_sts1_flds = {3, hsmcpcs2_sts1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_sts2_fld_list[] = {
    {"bercount",
     13,
     8,
     0,
     11297,
     "Counts bad sync headers (lower order bits). Latches high order bercount "
     "bits on read. Resets internal counter."},

    {"erroredblocks",
     7,
     0,
     0,
     11298,
     "Counts errored blocks (lower order bits). Latches high order "
     "erroredblocks bits on read. Resets internal counter."},

};
reg_decoder_t hsmcpcs2_sts2_flds = {2, hsmcpcs2_sts2_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatterna1_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11299, "Bits[15:0] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs2_testpatterna1_flds = {
    1, hsmcpcs2_testpatterna1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatterna2_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11300, "Bits[31:16] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs2_testpatterna2_flds = {
    1, hsmcpcs2_testpatterna2_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatterna3_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11301, "Bits[47:32] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs2_testpatterna3_flds = {
    1, hsmcpcs2_testpatterna3_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatterna4_fld_list[] = {
    {"testpatterna", 9, 0, 0, 11302, "Bits[57:48] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs2_testpatterna4_flds = {
    1, hsmcpcs2_testpatterna4_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatternb1_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11303, "Bits[15:0] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs2_testpatternb1_flds = {
    1, hsmcpcs2_testpatternb1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatternb2_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11304, "Bits[31:16] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs2_testpatternb2_flds = {
    1, hsmcpcs2_testpatternb2_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatternb3_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11305, "Bits[47:32] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs2_testpatternb3_flds = {
    1, hsmcpcs2_testpatternb3_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatternb4_fld_list[] = {
    {"testpatternb", 9, 0, 0, 11306, "Bits[57:48] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs2_testpatternb4_flds = {
    1, hsmcpcs2_testpatternb4_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatctrl_fld_list[] = {
    {"datapatternselect",
     0,
     0,
     0,
     11307,
     "Data input to pseudo random test mode. 0=Local Fault Data Pattern. "
     "1=Zeros Data Pattern"},

    {"testpatternselect",
     1,
     1,
     0,
     11308,
     "0=pseudo random test pattern. 1=square wave test pattern."},

    {"rtestmode", 2, 2, 0, 11309, "Enable receive test_pattern testing"},

    {"ttestmode", 3, 3, 0, 11310, "Enable transmit test_pattern"},

    {"scrambledidle",
     7,
     7,
     0,
     11311,
     "Enable scrambled idle test pattern mode"},

};
reg_decoder_t hsmcpcs2_testpatctrl_flds = {
    5, hsmcpcs2_testpatctrl_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_testpatternerrors_fld_list[] = {
    {"testpatternerrors",
     15,
     0,
     0,
     11312,
     "Counts test pattern reception errors"},

};
reg_decoder_t hsmcpcs2_testpatternerrors_flds = {
    1, hsmcpcs2_testpatternerrors_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_bercounthi_fld_list[] = {
    {"bercount",
     15,
     0,
     0,
     11313,
     "Counts bad sync headers (higher order bits). Cleared and latched when "
     "sts2 register is read."},

};
reg_decoder_t hsmcpcs2_bercounthi_flds = {1, hsmcpcs2_bercounthi_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_erroredblockshi_fld_list[] = {
    {"erroredblocks",
     13,
     0,
     0,
     11314,
     "Counts errored blocks (higher order bits). Cleared and latched when sts2 "
     "register is read."},

};
reg_decoder_t hsmcpcs2_erroredblockshi_flds = {
    1, hsmcpcs2_erroredblockshi_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_counters0_fld_list[] = {
    {"syncloss", 7, 0, 0, 11315, "Sync loss"},

    {"blocklockloss", 15, 8, 0, 11316, "Block lock loss"},

};
reg_decoder_t hsmcpcs2_counters0_flds = {2, hsmcpcs2_counters0_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_counters1_fld_list[] = {
    {"highber", 7, 0, 0, 11317, "High BER"},

    {"vlderr", 15, 8, 0, 11318, "Valid Error"},

};
reg_decoder_t hsmcpcs2_counters1_flds = {2, hsmcpcs2_counters1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_counters2_fld_list[] = {
    {"unkerr", 7, 0, 0, 11319, "Unknown Error"},

    {"invlderr", 15, 8, 0, 11320, "Invalid Error"},

};
reg_decoder_t hsmcpcs2_counters2_flds = {2, hsmcpcs2_counters2_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_algnstat1_fld_list[] = {
    {"blocklock", 3, 0, 0, 11321, "Block lock on lane x"},

    {"alignstatus", 12, 12, 0, 11322, "All lanes are syncronized and aligned"},

};
reg_decoder_t hsmcpcs2_algnstat1_flds = {2, hsmcpcs2_algnstat1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_algnstat3_fld_list[] = {
    {"amlock", 3, 0, 0, 11323, "Alignment marker lock on lane x"},

};
reg_decoder_t hsmcpcs2_algnstat3_flds = {1, hsmcpcs2_algnstat3_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_amctrl_fld_list[] = {
    {"enable",
     0,
     0,
     0,
     11324,
     "Enables the use of custom alignment markers for 25G mode for IEEE draft "
     "compatibility "},

    {"amenable25g",
     1,
     1,
     0,
     11325,
     "Enables the insertion of alignment markers in 25G mode  for IEEE draft "
     "compatibility "},

};
reg_decoder_t hsmcpcs2_amctrl_flds = {2, hsmcpcs2_amctrl_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_fint0_fld_list[] = {
    {"ovrdint", 9, 0, 0, 11326, "block lock Interrupt override"},

};
reg_decoder_t hsmcpcs2_fint0_flds = {1, hsmcpcs2_fint0_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_lanemapping0_fld_list[] = {
    {"lanemapping0",
     1,
     0,
     0,
     11327,
     "Indicates which PCS lane is received on lane 0"},

};
reg_decoder_t hsmcpcs2_lanemapping0_flds = {
    1, hsmcpcs2_lanemapping0_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_lanemapping1_fld_list[] = {
    {"lanemapping1",
     1,
     0,
     0,
     11328,
     "Indicates which PCS lane is received on lane 1"},

};
reg_decoder_t hsmcpcs2_lanemapping1_flds = {
    1, hsmcpcs2_lanemapping1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_lanemapping2_fld_list[] = {
    {"lanemapping2",
     1,
     0,
     0,
     11329,
     "Indicates which PCS lane is received on lane 2"},

};
reg_decoder_t hsmcpcs2_lanemapping2_flds = {
    1, hsmcpcs2_lanemapping2_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_lanemapping3_fld_list[] = {
    {"lanemapping3",
     1,
     0,
     0,
     11330,
     "Indicates which PCS lane is received on lane 3"},

};
reg_decoder_t hsmcpcs2_lanemapping3_flds = {
    1, hsmcpcs2_lanemapping3_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_biperrorsln0_fld_list[] = {
    {"biperrorsln0",
     15,
     0,
     0,
     11331,
     "Bit Interleaved Parity errors count on lane 0"},

};
reg_decoder_t hsmcpcs2_biperrorsln0_flds = {
    1, hsmcpcs2_biperrorsln0_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_biperrorsln1_fld_list[] = {
    {"biperrorsln1",
     15,
     0,
     0,
     11332,
     "Bit Interleaved Parity errors count on lane 1"},

};
reg_decoder_t hsmcpcs2_biperrorsln1_flds = {
    1, hsmcpcs2_biperrorsln1_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_biperrorsln2_fld_list[] = {
    {"biperrorsln2",
     15,
     0,
     0,
     11333,
     "Bit Interleaved Parity errors count on lane 2"},

};
reg_decoder_t hsmcpcs2_biperrorsln2_flds = {
    1, hsmcpcs2_biperrorsln2_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_biperrorsln3_fld_list[] = {
    {"biperrorsln3",
     15,
     0,
     0,
     11334,
     "Bit Interleaved Parity errors count on lane 3"},

};
reg_decoder_t hsmcpcs2_biperrorsln3_flds = {
    1, hsmcpcs2_biperrorsln3_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_dbgctrl2_fld_list[] = {
    {"dbgbiterroren", 1, 1, 0, 11335, "Enable bit error injection"},

    {"dbgsyncheaderen", 2, 2, 0, 11336, "Enable sync header corruption"},

    {"dbginsertcode",
     5,
     5,
     0,
     11337,
     "Send a single custom code word when this bit is asserted"},

};
reg_decoder_t hsmcpcs2_dbgctrl2_flds = {3, hsmcpcs2_dbgctrl2_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_dbgbersmovrd_fld_list[] = {
    {"dbgdisablebermonitor",
     0,
     0,
     0,
     11338,
     "Set to 1 disables BER state machine by holding the INIT state"},

    {"dbgusecustombervals", 1, 1, 0, 11339, "Set to 1 to use custom xus timer"},

    {"dbgcustomxustimerval",
     8,
     2,
     0,
     11340,
     "Override value for bits[15:9] of BER state machine xus timer, all other "
     "xus timer bits are set to 0 when used. Enabled by dbgusecustombervals."},

    {"dbgcustombadmaxval",
     15,
     9,
     0,
     11341,
     "Override value for the number of bad sync headers required to cause the "
     "hiBER case. Enabled by dbgusecustombervals."},

};
reg_decoder_t hsmcpcs2_dbgbersmovrd_flds = {
    4, hsmcpcs2_dbgbersmovrd_fld_list, 16};

reg_decoder_fld_t hsmcpcs2_dbgctrl1_fld_list[] = {
    {"dbgfullthreshold",
     9,
     6,
     0,
     11342,
     "Maximum number of entries that may be stored in the TX gearbox async "
     "FIFOs at one time. Allowable values 0-8."},

    {"debuglowlatencymode",
     13,
     13,
     0,
     11343,
     "Assumes data to be error free in order to improve latency. VIOLATES "
     "IEEE."},

};
reg_decoder_t hsmcpcs2_dbgctrl1_flds = {2, hsmcpcs2_dbgctrl1_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_ctrl1_fld_list[] = {
    {"reset",
     15,
     15,
     0,
     11344,
     "Holds the PCS/FEC Channel in software reset    1'b1 : Reset    1'b0 : "
     "Normal Operation"},

};
reg_decoder_t hsmcpcs3_ctrl1_flds = {1, hsmcpcs3_ctrl1_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_status1_fld_list[] = {
    {"fault", 7, 7, 0, 11345, "When asserted, a fault condition is detected."},

};
reg_decoder_t hsmcpcs3_status1_flds = {1, hsmcpcs3_status1_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_mode_fld_list[] = {
    {"mlg",
     13,
     10,
     0,
     11346,
     "MLG Channel Mapping: 4'b1111 - 4'b1101 : Reserved 4'b1100:  40G slot 1 "
     "4'b1011 : 40G slot 0 4'b1010 : 10G slot 9 4'b1001: 10G slot 8 4'b1000: "
     "10G slot 7 4'b0111: 10G slot 6 4'b0110: 10G slot 5 4'b0101: 10G slot 4 "
     "4'b0100: 10G slot 3 4'b0011: 10G slot 2 4'b0010: 10G slot 1 4'b0001: 10G "
     "slot 0 4'b0000 : Disabled"},

    {"pma",
     5,
     2,
     0,
     11347,
     "4'b1111 : MLG  4'b1110 : Fiber Channel 4'b1101 - 4'b1000 : Reserved "
     "4'b0111 : BASE-R10 4'b0110 : BASE-R8 4'b0101 : BASE-R4 4'b0100 : BASE-R2 "
     "4'b0011 : BASE-R1 / BASE-X 4'b0010 : QSGMII 4'b0001 : SGMII 4'b0000 : "
     "Disabled"},

    {"fec",
     1,
     0,
     0,
     11348,
     "2'b11 : Reserved 2'b10 : RSFEC 2'b01 : FCFEC 2'b00 : None"},

};
reg_decoder_t hsmcpcs3_mode_flds = {3, hsmcpcs3_mode_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_sts1_fld_list[] = {
    {"pcsstatus",
     12,
     12,
     0,
     11349,
     "When asserted PCS is in a fully operational state"},

    {"hiber", 1, 1, 0, 11350, "High Bit Error Rate detected"},

    {"blocklockall", 0, 0, 0, 11351, "PCS locked to received blocks"},

};
reg_decoder_t hsmcpcs3_sts1_flds = {3, hsmcpcs3_sts1_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_sts2_fld_list[] = {
    {"bercount",
     13,
     8,
     0,
     11352,
     "Counts bad sync headers (lower order bits). Latches high order bercount "
     "bits on read. Resets internal counter."},

    {"erroredblocks",
     7,
     0,
     0,
     11353,
     "Counts errored blocks (lower order bits). Latches high order "
     "erroredblocks bits on read. Resets internal counter."},

};
reg_decoder_t hsmcpcs3_sts2_flds = {2, hsmcpcs3_sts2_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatterna1_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11354, "Bits[15:0] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs3_testpatterna1_flds = {
    1, hsmcpcs3_testpatterna1_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatterna2_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11355, "Bits[31:16] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs3_testpatterna2_flds = {
    1, hsmcpcs3_testpatterna2_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatterna3_fld_list[] = {
    {"testpatterna", 15, 0, 0, 11356, "Bits[47:32] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs3_testpatterna3_flds = {
    1, hsmcpcs3_testpatterna3_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatterna4_fld_list[] = {
    {"testpatterna", 9, 0, 0, 11357, "Bits[57:48] of seed A of test pattern"},

};
reg_decoder_t hsmcpcs3_testpatterna4_flds = {
    1, hsmcpcs3_testpatterna4_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatternb1_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11358, "Bits[15:0] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs3_testpatternb1_flds = {
    1, hsmcpcs3_testpatternb1_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatternb2_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11359, "Bits[31:16] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs3_testpatternb2_flds = {
    1, hsmcpcs3_testpatternb2_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatternb3_fld_list[] = {
    {"testpatternb", 15, 0, 0, 11360, "Bits[47:32] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs3_testpatternb3_flds = {
    1, hsmcpcs3_testpatternb3_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatternb4_fld_list[] = {
    {"testpatternb", 9, 0, 0, 11361, "Bits[57:48] of seed B of test pattern"},

};
reg_decoder_t hsmcpcs3_testpatternb4_flds = {
    1, hsmcpcs3_testpatternb4_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatctrl_fld_list[] = {
    {"datapatternselect",
     0,
     0,
     0,
     11362,
     "Data input to pseudo random test mode. 0=Local Fault Data Pattern. "
     "1=Zeros Data Pattern"},

    {"testpatternselect",
     1,
     1,
     0,
     11363,
     "0=pseudo random test pattern. 1=square wave test pattern."},

    {"rtestmode", 2, 2, 0, 11364, "Enable receive test_pattern testing"},

    {"ttestmode", 3, 3, 0, 11365, "Enable transmit test_pattern"},

    {"scrambledidle",
     7,
     7,
     0,
     11366,
     "Enable scrambled idle test pattern mode"},

};
reg_decoder_t hsmcpcs3_testpatctrl_flds = {
    5, hsmcpcs3_testpatctrl_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_testpatternerrors_fld_list[] = {
    {"testpatternerrors",
     15,
     0,
     0,
     11367,
     "Counts test pattern reception errors"},

};
reg_decoder_t hsmcpcs3_testpatternerrors_flds = {
    1, hsmcpcs3_testpatternerrors_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_bercounthi_fld_list[] = {
    {"bercount",
     15,
     0,
     0,
     11368,
     "Counts bad sync headers (higher order bits). Cleared and latched when "
     "sts2 register is read."},

};
reg_decoder_t hsmcpcs3_bercounthi_flds = {1, hsmcpcs3_bercounthi_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_erroredblockshi_fld_list[] = {
    {"erroredblocks",
     13,
     0,
     0,
     11369,
     "Counts errored blocks (higher order bits). Cleared and latched when sts2 "
     "register is read."},

};
reg_decoder_t hsmcpcs3_erroredblockshi_flds = {
    1, hsmcpcs3_erroredblockshi_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_counters0_fld_list[] = {
    {"syncloss", 7, 0, 0, 11370, "Sync loss"},

    {"blocklockloss", 15, 8, 0, 11371, "Block lock loss"},

};
reg_decoder_t hsmcpcs3_counters0_flds = {2, hsmcpcs3_counters0_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_counters1_fld_list[] = {
    {"highber", 7, 0, 0, 11372, "High BER"},

    {"vlderr", 15, 8, 0, 11373, "Valid Error"},

};
reg_decoder_t hsmcpcs3_counters1_flds = {2, hsmcpcs3_counters1_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_counters2_fld_list[] = {
    {"unkerr", 7, 0, 0, 11374, "Unknown Error"},

    {"invlderr", 15, 8, 0, 11375, "Invalid Error"},

};
reg_decoder_t hsmcpcs3_counters2_flds = {2, hsmcpcs3_counters2_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_algnstat1_fld_list[] = {
    {"blocklock", 0, 0, 0, 11376, "Block lock on lane x"},

};
reg_decoder_t hsmcpcs3_algnstat1_flds = {1, hsmcpcs3_algnstat1_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_amctrl_fld_list[] = {
    {"enable",
     0,
     0,
     0,
     11377,
     "Enables the use of custom alignment markers for 25G mode for IEEE draft "
     "compatibility "},

    {"amenable25g",
     1,
     1,
     0,
     11378,
     "Enables the insertion of alignment markers in 25G mode  for IEEE draft "
     "compatibility "},

};
reg_decoder_t hsmcpcs3_amctrl_flds = {2, hsmcpcs3_amctrl_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_fint0_fld_list[] = {
    {"ovrdint", 9, 0, 0, 11379, "block lock Interrupt override"},

};
reg_decoder_t hsmcpcs3_fint0_flds = {1, hsmcpcs3_fint0_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_dbgctrl2_fld_list[] = {
    {"dbgbiterroren", 1, 1, 0, 11380, "Enable bit error injection"},

    {"dbgsyncheaderen", 2, 2, 0, 11381, "Enable sync header corruption"},

    {"dbginsertcode",
     5,
     5,
     0,
     11382,
     "Send a single custom code word when this bit is asserted"},

};
reg_decoder_t hsmcpcs3_dbgctrl2_flds = {3, hsmcpcs3_dbgctrl2_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_dbgbersmovrd_fld_list[] = {
    {"dbgdisablebermonitor",
     0,
     0,
     0,
     11383,
     "Set to 1 disables BER state machine by holding the INIT state"},

    {"dbgusecustombervals", 1, 1, 0, 11384, "Set to 1 to use custom xus timer"},

    {"dbgcustomxustimerval",
     8,
     2,
     0,
     11385,
     "Override value for bits[15:9] of BER state machine xus timer, all other "
     "xus timer bits are set to 0 when used. Enabled by dbgusecustombervals."},

    {"dbgcustombadmaxval",
     15,
     9,
     0,
     11386,
     "Override value for the number of bad sync headers required to cause the "
     "hiBER case. Enabled by dbgusecustombervals."},

};
reg_decoder_t hsmcpcs3_dbgbersmovrd_flds = {
    4, hsmcpcs3_dbgbersmovrd_fld_list, 16};

reg_decoder_fld_t hsmcpcs3_dbgctrl1_fld_list[] = {
    {"dbgfullthreshold",
     9,
     6,
     0,
     11387,
     "Maximum number of entries that may be stored in the TX gearbox async "
     "FIFOs at one time. Allowable values 0-8."},

    {"debuglowlatencymode",
     13,
     13,
     0,
     11388,
     "Assumes data to be error free in order to improve latency. VIOLATES "
     "IEEE."},

};
reg_decoder_t hsmcpcs3_dbgctrl1_flds = {2, hsmcpcs3_dbgctrl1_fld_list, 16};

reg_decoder_fld_t fecrs0_ctrl_fld_list[] = {
    {"bypcorrena",
     0,
     0,
     0,
     11389,
     "FEC bypass correction enable 1 = FEC decoder performs detection without "
     "correction 0 = FEC decoder performs detection and correction"},

    {"bypindiena",
     1,
     1,
     0,
     11390,
     "FEC bypass error indication 1 = FEC decoder does not indicate errors 0 = "
     "FEC decoder indicates errors to the PCS layer"},

};
reg_decoder_t fecrs0_ctrl_flds = {2, fecrs0_ctrl_fld_list, 16};

reg_decoder_fld_t fecrs0_sts_fld_list[] = {
    {"bypcorrabi",
     0,
     0,
     0,
     11391,
     "FEC bypass correction ability 1 = FEC decoder has ability to bypass "
     "error correction 0 = FEC decoder does not have correction bypass "
     "ability"},

    {"bypindiabi",
     1,
     1,
     0,
     11392,
     "FEC bypass indication ability 1 = FEC decoder has ability to bypass "
     "error indication to PCS 0 = FEC decoder always indicates errors to PCS"},

    {"hiser",
     2,
     2,
     0,
     11393,
     "hi_ser 1 = number of FEC symol errors over 8192 codewords exceeds "
     "threshold (KR4=417) 0 = number of FEC symol errors over 8192 codewords "
     "below threshold"},

    {"alignstatus",
     14,
     14,
     0,
     11394,
     "FEC align status 1: all lanes are synchronized and aligned 0: Deskew "
     "process not complete"},

};
reg_decoder_t fecrs0_sts_flds = {4, fecrs0_sts_fld_list, 16};

reg_decoder_fld_t fecrs0_corrcntlo_fld_list[] = {
    {"corrcnt",
     15,
     0,
     0,
     11395,
     "FEC_corrected_blocks_counter low (Lower 16 bits)"},

};
reg_decoder_t fecrs0_corrcntlo_flds = {1, fecrs0_corrcntlo_fld_list, 16};

reg_decoder_fld_t fecrs0_corrcnthi_fld_list[] = {
    {"corrcnt",
     15,
     0,
     0,
     11396,
     "FEC_corrected_blocks_counter High (Upper 16 bits)"},

};
reg_decoder_t fecrs0_corrcnthi_flds = {1, fecrs0_corrcnthi_fld_list, 16};

reg_decoder_fld_t fecrs0_uncorrcntlo_fld_list[] = {
    {"uncorrcnt",
     15,
     0,
     0,
     11397,
     " FEC_uncorrected_blocks_counter low (Lower 16 bits)"},

};
reg_decoder_t fecrs0_uncorrcntlo_flds = {1, fecrs0_uncorrcntlo_fld_list, 16};

reg_decoder_fld_t fecrs0_uncorrcnthi_fld_list[] = {
    {"uncorrcnt",
     15,
     0,
     0,
     11398,
     "FEC_uncorrected_blocks_counter High (Upper 16 bits)"},

};
reg_decoder_t fecrs0_uncorrcnthi_flds = {1, fecrs0_uncorrcnthi_fld_list, 16};

reg_decoder_fld_t fecrs0_lanemapping_fld_list[] = {
    {"lane0intmapnum", 9, 8, 0, 11399, "Lane 0 internal mapped number"},

    {"lane1intmapnum", 11, 10, 0, 11400, "Lane 1 internal mapped number"},

    {"lane2intmapnum", 13, 12, 0, 11401, "Lane 2 internal mapped number"},

    {"lane3intmapnum", 15, 14, 0, 11402, "Lane 3 internal mapped number"},

};
reg_decoder_t fecrs0_lanemapping_flds = {4, fecrs0_lanemapping_fld_list, 16};

reg_decoder_fld_t fecrs0_serlane0lo_fld_list[] = {
    {"serlane0",
     15,
     0,
     0,
     11403,
     "FEC symbol errors, FEC lanes 0 low Lower 16 bits"},

};
reg_decoder_t fecrs0_serlane0lo_flds = {1, fecrs0_serlane0lo_fld_list, 16};

reg_decoder_fld_t fecrs0_serlane0hi_fld_list[] = {
    {"serlane0",
     15,
     0,
     0,
     11404,
     "FEC symbol errors, FEC lanes 0 high Upper 16 bits"},

};
reg_decoder_t fecrs0_serlane0hi_flds = {1, fecrs0_serlane0hi_fld_list, 16};

reg_decoder_fld_t fecrs0_serlane1lo_fld_list[] = {
    {"serlane1",
     15,
     0,
     0,
     11405,
     "FEC symbol errors, FEC lanes 1low Lower 16 bits"},

};
reg_decoder_t fecrs0_serlane1lo_flds = {1, fecrs0_serlane1lo_fld_list, 16};

reg_decoder_fld_t fecrs0_serlane1hi_fld_list[] = {
    {"serlane1",
     15,
     0,
     0,
     11406,
     "FEC symbol errors, FEC lanes 1high Upper 16 bits"},

};
reg_decoder_t fecrs0_serlane1hi_flds = {1, fecrs0_serlane1hi_fld_list, 16};

reg_decoder_fld_t fecrs0_serlane2lo_fld_list[] = {
    {"serlane2",
     15,
     0,
     0,
     11407,
     "FEC symbol errors, FEC lanes 2low Lower 16 bits"},

};
reg_decoder_t fecrs0_serlane2lo_flds = {1, fecrs0_serlane2lo_fld_list, 16};

reg_decoder_fld_t fecrs0_serlane2hi_fld_list[] = {
    {"serlane2",
     15,
     0,
     0,
     11408,
     "FEC symbol errors, FEC lanes 2high Upper 16 bits"},

};
reg_decoder_t fecrs0_serlane2hi_flds = {1, fecrs0_serlane2hi_fld_list, 16};

reg_decoder_fld_t fecrs0_serlane3lo_fld_list[] = {
    {"serlane3",
     15,
     0,
     0,
     11409,
     "FEC symbol errors, FEC lanes 3low Lower 16 bits"},

};
reg_decoder_t fecrs0_serlane3lo_flds = {1, fecrs0_serlane3lo_fld_list, 16};

reg_decoder_fld_t fecrs0_serlane3hi_fld_list[] = {
    {"serlane3",
     15,
     0,
     0,
     11410,
     "FEC symbol errors, FEC lanes 3high Upper 16 bits"},

};
reg_decoder_t fecrs0_serlane3hi_flds = {1, fecrs0_serlane3hi_fld_list, 16};

reg_decoder_fld_t fecrs0_dbgctrl_fld_list[] = {
    {"softreset",
     0,
     0,
     0,
     11411,
     "Reset active high 1: reset 0: normal operation"},

    {"disablefec",
     1,
     1,
     0,
     11412,
     "1: disable FEC 0: enable FEC (normal operation)"},

    {"debuguseshortamp",
     2,
     2,
     0,
     11413,
     "Use short alignment marker. Internal use only."},

    {"debugswtestint",
     3,
     3,
     0,
     11414,
     "Software test interrupt. Internal use only."},

    {"debugcwteststoponfaildis", 4, 4, 0, 11415, "Internal use only. "},

    {"alwaysusecl49",
     6,
     6,
     0,
     11416,
     "When set to 1, always allow clause 49 block types."},

    {"neverusecl49",
     7,
     7,
     0,
     11417,
     "When set to 1, never allow clause 49 block types. It overrides "
     "alwaysusecl49."},

    {"pcsscrena",
     8,
     8,
     0,
     11418,
     "When set to 1, enables channelized pcs scrambler decoding."},

    {"mlgscrena",
     9,
     9,
     0,
     11419,
     "When set to 1, enables 100G aggragate mlg scrambler decoding. "},

};
reg_decoder_t fecrs0_dbgctrl_flds = {9, fecrs0_dbgctrl_fld_list, 16};

reg_decoder_fld_t fecrs0_dbgsts_fld_list[] = {
    {"Interrupt", 0, 0, 0, 11420, "Logical OR of all interrupts."},

    {"amplockstat",
     7,
     4,
     0,
     11421,
     "Alignment marker lock status per each lane"},

};
reg_decoder_t fecrs0_dbgsts_flds = {2, fecrs0_dbgsts_fld_list, 16};

reg_decoder_fld_t fecrs0_dbgberintthres_fld_list[] = {
    {"dbgberintthres",
     15,
     0,
     0,
     11422,
     "Specify BER Interrupt Threshold value"},

};
reg_decoder_t fecrs0_dbgberintthres_flds = {
    1, fecrs0_dbgberintthres_fld_list, 16};

reg_decoder_fld_t fecrs0_frcinvalidtimerlo_fld_list[] = {
    {"frcinvalidtimer",
     15,
     0,
     0,
     11423,
     "Amount of time FEC should force invalid Sync Headers when when "
     "correction_enable=1, bypass_indication=1 and hi_ser condition has been "
     "met (60-75ms)"},

};
reg_decoder_t fecrs0_frcinvalidtimerlo_flds = {
    1, fecrs0_frcinvalidtimerlo_fld_list, 16};

reg_decoder_fld_t fecrs0_frcinvalidtimerhi_fld_list[] = {
    {"frcinvalidtimer",
     15,
     0,
     0,
     11424,
     "Amount of time FEC should force invalid Sync Headers when when "
     "bypass_correction_enable=1, bypass_indication=1 and hi_ser condition has "
     "been met (60-75ms)"},

};
reg_decoder_t fecrs0_frcinvalidtimerhi_flds = {
    1, fecrs0_frcinvalidtimerhi_fld_list, 16};

reg_decoder_fld_t fecrs0_fint0_fld_list[] = {
    {"ovrdint", 6, 0, 0, 11425, "FEC AM lost lane 0 Interrupt override"},

};
reg_decoder_t fecrs0_fint0_flds = {1, fecrs0_fint0_fld_list, 16};

reg_decoder_fld_t fecrs0_spare0_fld_list[] = {
    {"spare0", 7, 0, 0, 11426, "Spare0"},

};
reg_decoder_t fecrs0_spare0_flds = {1, fecrs0_spare0_fld_list, 16};

reg_decoder_fld_t fecrs0_ieeecfg_fld_list[] = {
    {"blena74",
     0,
     0,
     0,
     11427,
     "Enable use of clause 74-style block lock without alignment markers (For "
     "IEEE draft compatibility)"},

    {"blrobust",
     3,
     1,
     0,
     11428,
     "Enables robust block lock. 3'b000 gains and loses lock based on good and "
     "bad codewords. 3'b111 gains and loses lcok based on correctable and "
     "uncorrectable codewords. 3'b011: waits for 1 good codeword, then uses "
     "correctable/uncorrectable. 3'b001 gains lock based .  on good codewords "
     "and loses lock based on uncorrectable codewords.  Only valid when Clause "
     "74 block lock is in use."},

    {"blgainth",
     5,
     4,
     0,
     11429,
     "Number of good/correctable consectutive blocks (-1) to see before "
     "locking.  Only valid when Clause 74 block lock is in use."},

    {"bllossth",
     8,
     6,
     0,
     11430,
     "Number of bad/uncorrectable consectutive blocks (-1) to see before "
     "losing lock. Only valid when Clause 74 block lock is in use."},

    {"padval",
     11,
     11,
     0,
     11431,
     "Pad value of 25G when transcoding alignment markers."},

    {"enascr", 12, 12, 0, 11432, "Enable Transcoding Scrambler"},

    {"enapn", 13, 13, 0, 11433, "Enable PN-2112/5280 Scrambling"},

    {"enaindi2",
     14,
     14,
     0,
     11434,
     "Enables sync header corruption on the 2nd 257-bit block on a "
     "uncorrectable codeword.  (For IEEE draft compatibility)"},

    {"enaindi6",
     15,
     15,
     0,
     11435,
     "Enables sync header corruption on the 6th 257-bit block on a "
     "uncorrectable codeword.  (For IEEE draft compatibility)"},

};
reg_decoder_t fecrs0_ieeecfg_flds = {9, fecrs0_ieeecfg_fld_list, 16};

reg_decoder_fld_t fecrs1_ctrl_fld_list[] = {
    {"bypcorrena",
     0,
     0,
     0,
     11436,
     "FEC bypass correction enable 1 = FEC decoder performs detection without "
     "correction 0 = FEC decoder performs detection and correction"},

    {"bypindiena",
     1,
     1,
     0,
     11437,
     "FEC bypass error indication 1 = FEC decoder does not indicate errors 0 = "
     "FEC decoder indicates errors to the PCS layer"},

};
reg_decoder_t fecrs1_ctrl_flds = {2, fecrs1_ctrl_fld_list, 16};

reg_decoder_fld_t fecrs1_sts_fld_list[] = {
    {"bypcorrabi",
     0,
     0,
     0,
     11438,
     "FEC bypass correction ability 1 = FEC decoder has ability to bypass "
     "error correction 0 = FEC decoder does not have correction bypass "
     "ability"},

    {"bypindiabi",
     1,
     1,
     0,
     11439,
     "FEC bypass indication ability 1 = FEC decoder has ability to bypass "
     "error indication to PCS 0 = FEC decoder always indicates errors to PCS"},

    {"hiser",
     2,
     2,
     0,
     11440,
     "hi_ser 1 = number of FEC symol errors over 8192 codewords exceeds "
     "threshold (KR4=417) 0 = number of FEC symol errors over 8192 codewords "
     "below threshold"},

    {"alignstatus",
     14,
     14,
     0,
     11441,
     "FEC align status 1: all lanes are synchronized and aligned 0: Deskew "
     "process not complete"},

};
reg_decoder_t fecrs1_sts_flds = {4, fecrs1_sts_fld_list, 16};

reg_decoder_fld_t fecrs1_corrcntlo_fld_list[] = {
    {"corrcnt",
     15,
     0,
     0,
     11442,
     "FEC_corrected_blocks_counter low (Lower 16 bits)"},

};
reg_decoder_t fecrs1_corrcntlo_flds = {1, fecrs1_corrcntlo_fld_list, 16};

reg_decoder_fld_t fecrs1_corrcnthi_fld_list[] = {
    {"corrcnt",
     15,
     0,
     0,
     11443,
     "FEC_corrected_blocks_counter High (Upper 16 bits)"},

};
reg_decoder_t fecrs1_corrcnthi_flds = {1, fecrs1_corrcnthi_fld_list, 16};

reg_decoder_fld_t fecrs1_uncorrcntlo_fld_list[] = {
    {"uncorrcnt",
     15,
     0,
     0,
     11444,
     " FEC_uncorrected_blocks_counter low (Lower 16 bits)"},

};
reg_decoder_t fecrs1_uncorrcntlo_flds = {1, fecrs1_uncorrcntlo_fld_list, 16};

reg_decoder_fld_t fecrs1_uncorrcnthi_fld_list[] = {
    {"uncorrcnt",
     15,
     0,
     0,
     11445,
     "FEC_uncorrected_blocks_counter High (Upper 16 bits)"},

};
reg_decoder_t fecrs1_uncorrcnthi_flds = {1, fecrs1_uncorrcnthi_fld_list, 16};

reg_decoder_fld_t fecrs1_lanemapping_fld_list[] = {
    {"lane0intmapnum", 9, 8, 0, 11446, "Lane 0 internal mapped number"},

    {"lane1intmapnum", 11, 10, 0, 11447, "Lane 1 internal mapped number"},

    {"lane2intmapnum", 13, 12, 0, 11448, "Lane 2 internal mapped number"},

    {"lane3intmapnum", 15, 14, 0, 11449, "Lane 3 internal mapped number"},

};
reg_decoder_t fecrs1_lanemapping_flds = {4, fecrs1_lanemapping_fld_list, 16};

reg_decoder_fld_t fecrs1_serlane0lo_fld_list[] = {
    {"serlane0",
     15,
     0,
     0,
     11450,
     "FEC symbol errors, FEC lanes 0 low Lower 16 bits"},

};
reg_decoder_t fecrs1_serlane0lo_flds = {1, fecrs1_serlane0lo_fld_list, 16};

reg_decoder_fld_t fecrs1_serlane0hi_fld_list[] = {
    {"serlane0",
     15,
     0,
     0,
     11451,
     "FEC symbol errors, FEC lanes 0 high Upper 16 bits"},

};
reg_decoder_t fecrs1_serlane0hi_flds = {1, fecrs1_serlane0hi_fld_list, 16};

reg_decoder_fld_t fecrs1_serlane1lo_fld_list[] = {
    {"serlane1",
     15,
     0,
     0,
     11452,
     "FEC symbol errors, FEC lanes 1low Lower 16 bits"},

};
reg_decoder_t fecrs1_serlane1lo_flds = {1, fecrs1_serlane1lo_fld_list, 16};

reg_decoder_fld_t fecrs1_serlane1hi_fld_list[] = {
    {"serlane1",
     15,
     0,
     0,
     11453,
     "FEC symbol errors, FEC lanes 1high Upper 16 bits"},

};
reg_decoder_t fecrs1_serlane1hi_flds = {1, fecrs1_serlane1hi_fld_list, 16};

reg_decoder_fld_t fecrs1_serlane2lo_fld_list[] = {
    {"serlane2",
     15,
     0,
     0,
     11454,
     "FEC symbol errors, FEC lanes 2low Lower 16 bits"},

};
reg_decoder_t fecrs1_serlane2lo_flds = {1, fecrs1_serlane2lo_fld_list, 16};

reg_decoder_fld_t fecrs1_serlane2hi_fld_list[] = {
    {"serlane2",
     15,
     0,
     0,
     11455,
     "FEC symbol errors, FEC lanes 2high Upper 16 bits"},

};
reg_decoder_t fecrs1_serlane2hi_flds = {1, fecrs1_serlane2hi_fld_list, 16};

reg_decoder_fld_t fecrs1_serlane3lo_fld_list[] = {
    {"serlane3",
     15,
     0,
     0,
     11456,
     "FEC symbol errors, FEC lanes 3low Lower 16 bits"},

};
reg_decoder_t fecrs1_serlane3lo_flds = {1, fecrs1_serlane3lo_fld_list, 16};

reg_decoder_fld_t fecrs1_serlane3hi_fld_list[] = {
    {"serlane3",
     15,
     0,
     0,
     11457,
     "FEC symbol errors, FEC lanes 3high Upper 16 bits"},

};
reg_decoder_t fecrs1_serlane3hi_flds = {1, fecrs1_serlane3hi_fld_list, 16};

reg_decoder_fld_t fecrs1_dbgctrl_fld_list[] = {
    {"softreset",
     0,
     0,
     0,
     11458,
     "Reset active high 1: reset 0: normal operation"},

    {"disablefec",
     1,
     1,
     0,
     11459,
     "1: disable FEC 0: enable FEC (normal operation)"},

    {"debuguseshortamp",
     2,
     2,
     0,
     11460,
     "Use short alignment marker. Internal use only."},

    {"debugswtestint",
     3,
     3,
     0,
     11461,
     "Software test interrupt. Internal use only."},

    {"debugcwteststoponfaildis", 4, 4, 0, 11462, "Internal use only. "},

    {"alwaysusecl49",
     6,
     6,
     0,
     11463,
     "When set to 1, always allow clause 49 block types."},

    {"neverusecl49",
     7,
     7,
     0,
     11464,
     "When set to 1, never allow clause 49 block types. It overrides "
     "alwaysusecl49."},

    {"pcsscrena",
     8,
     8,
     0,
     11465,
     "When set to 1, enables channelized pcs scrambler decoding."},

    {"mlgscrena",
     9,
     9,
     0,
     11466,
     "When set to 1, enables 100G aggragate mlg scrambler decoding. "},

};
reg_decoder_t fecrs1_dbgctrl_flds = {9, fecrs1_dbgctrl_fld_list, 16};

reg_decoder_fld_t fecrs1_dbgsts_fld_list[] = {
    {"Interrupt", 0, 0, 0, 11467, "Logical OR of all interrupts."},

    {"amplockstat",
     7,
     4,
     0,
     11468,
     "Alignment marker lock status per each lane"},

};
reg_decoder_t fecrs1_dbgsts_flds = {2, fecrs1_dbgsts_fld_list, 16};

reg_decoder_fld_t fecrs1_dbgberintthres_fld_list[] = {
    {"dbgberintthres",
     15,
     0,
     0,
     11469,
     "Specify BER Interrupt Threshold value"},

};
reg_decoder_t fecrs1_dbgberintthres_flds = {
    1, fecrs1_dbgberintthres_fld_list, 16};

reg_decoder_fld_t fecrs1_frcinvalidtimerlo_fld_list[] = {
    {"frcinvalidtimer",
     15,
     0,
     0,
     11470,
     "Amount of time FEC should force invalid Sync Headers when when "
     "correction_enable=1, bypass_indication=1 and hi_ser condition has been "
     "met (60-75ms)"},

};
reg_decoder_t fecrs1_frcinvalidtimerlo_flds = {
    1, fecrs1_frcinvalidtimerlo_fld_list, 16};

reg_decoder_fld_t fecrs1_frcinvalidtimerhi_fld_list[] = {
    {"frcinvalidtimer",
     15,
     0,
     0,
     11471,
     "Amount of time FEC should force invalid Sync Headers when when "
     "bypass_correction_enable=1, bypass_indication=1 and hi_ser condition has "
     "been met (60-75ms)"},

};
reg_decoder_t fecrs1_frcinvalidtimerhi_flds = {
    1, fecrs1_frcinvalidtimerhi_fld_list, 16};

reg_decoder_fld_t fecrs1_fint0_fld_list[] = {
    {"ovrdint", 6, 0, 0, 11472, "FEC AM lost lane 0 Interrupt override"},

};
reg_decoder_t fecrs1_fint0_flds = {1, fecrs1_fint0_fld_list, 16};

reg_decoder_fld_t fecrs1_spare0_fld_list[] = {
    {"spare0", 7, 0, 0, 11473, "Spare0"},

};
reg_decoder_t fecrs1_spare0_flds = {1, fecrs1_spare0_fld_list, 16};

reg_decoder_fld_t fecrs1_ieeecfg_fld_list[] = {
    {"blena74",
     0,
     0,
     0,
     11474,
     "Enable use of clause 74-style block lock without alignment markers (For "
     "IEEE draft compatibility)"},

    {"blrobust",
     3,
     1,
     0,
     11475,
     "Enables robust block lock. 3'b000 gains and loses lock based on good and "
     "bad codewords. 3'b111 gains and loses lcok based on correctable and "
     "uncorrectable codewords. 3'b011: waits for 1 good codeword, then uses "
     "correctable/uncorrectable. 3'b001 gains lock based .  on good codewords "
     "and loses lock based on uncorrectable codewords.  Only valid when Clause "
     "74 block lock is in use."},

    {"blgainth",
     5,
     4,
     0,
     11476,
     "Number of good/correctable consectutive blocks (-1) to see before "
     "locking.  Only valid when Clause 74 block lock is in use."},

    {"bllossth",
     8,
     6,
     0,
     11477,
     "Number of bad/uncorrectable consectutive blocks (-1) to see before "
     "losing lock. Only valid when Clause 74 block lock is in use."},

    {"padval",
     11,
     11,
     0,
     11478,
     "Pad value of 25G when transcoding alignment markers."},

    {"enascr", 12, 12, 0, 11479, "Enable Transcoding Scrambler"},

    {"enapn", 13, 13, 0, 11480, "Enable PN-2112/5280 Scrambling"},

    {"enaindi2",
     14,
     14,
     0,
     11481,
     "Enables sync header corruption on the 2nd 257-bit block on a "
     "uncorrectable codeword.  (For IEEE draft compatibility)"},

    {"enaindi6",
     15,
     15,
     0,
     11482,
     "Enables sync header corruption on the 6th 257-bit block on a "
     "uncorrectable codeword.  (For IEEE draft compatibility)"},

};
reg_decoder_t fecrs1_ieeecfg_flds = {9, fecrs1_ieeecfg_fld_list, 16};

reg_decoder_fld_t fecrs2_ctrl_fld_list[] = {
    {"bypcorrena",
     0,
     0,
     0,
     11483,
     "FEC bypass correction enable 1 = FEC decoder performs detection without "
     "correction 0 = FEC decoder performs detection and correction"},

    {"bypindiena",
     1,
     1,
     0,
     11484,
     "FEC bypass error indication 1 = FEC decoder does not indicate errors 0 = "
     "FEC decoder indicates errors to the PCS layer"},

};
reg_decoder_t fecrs2_ctrl_flds = {2, fecrs2_ctrl_fld_list, 16};

reg_decoder_fld_t fecrs2_sts_fld_list[] = {
    {"bypcorrabi",
     0,
     0,
     0,
     11485,
     "FEC bypass correction ability 1 = FEC decoder has ability to bypass "
     "error correction 0 = FEC decoder does not have correction bypass "
     "ability"},

    {"bypindiabi",
     1,
     1,
     0,
     11486,
     "FEC bypass indication ability 1 = FEC decoder has ability to bypass "
     "error indication to PCS 0 = FEC decoder always indicates errors to PCS"},

    {"hiser",
     2,
     2,
     0,
     11487,
     "hi_ser 1 = number of FEC symol errors over 8192 codewords exceeds "
     "threshold (KR4=417) 0 = number of FEC symol errors over 8192 codewords "
     "below threshold"},

    {"alignstatus",
     14,
     14,
     0,
     11488,
     "FEC align status 1: all lanes are synchronized and aligned 0: Deskew "
     "process not complete"},

};
reg_decoder_t fecrs2_sts_flds = {4, fecrs2_sts_fld_list, 16};

reg_decoder_fld_t fecrs2_corrcntlo_fld_list[] = {
    {"corrcnt",
     15,
     0,
     0,
     11489,
     "FEC_corrected_blocks_counter low (Lower 16 bits)"},

};
reg_decoder_t fecrs2_corrcntlo_flds = {1, fecrs2_corrcntlo_fld_list, 16};

reg_decoder_fld_t fecrs2_corrcnthi_fld_list[] = {
    {"corrcnt",
     15,
     0,
     0,
     11490,
     "FEC_corrected_blocks_counter High (Upper 16 bits)"},

};
reg_decoder_t fecrs2_corrcnthi_flds = {1, fecrs2_corrcnthi_fld_list, 16};

reg_decoder_fld_t fecrs2_uncorrcntlo_fld_list[] = {
    {"uncorrcnt",
     15,
     0,
     0,
     11491,
     " FEC_uncorrected_blocks_counter low (Lower 16 bits)"},

};
reg_decoder_t fecrs2_uncorrcntlo_flds = {1, fecrs2_uncorrcntlo_fld_list, 16};

reg_decoder_fld_t fecrs2_uncorrcnthi_fld_list[] = {
    {"uncorrcnt",
     15,
     0,
     0,
     11492,
     "FEC_uncorrected_blocks_counter High (Upper 16 bits)"},

};
reg_decoder_t fecrs2_uncorrcnthi_flds = {1, fecrs2_uncorrcnthi_fld_list, 16};

reg_decoder_fld_t fecrs2_lanemapping_fld_list[] = {
    {"lane0intmapnum", 9, 8, 0, 11493, "Lane 0 internal mapped number"},

    {"lane1intmapnum", 11, 10, 0, 11494, "Lane 1 internal mapped number"},

    {"lane2intmapnum", 13, 12, 0, 11495, "Lane 2 internal mapped number"},

    {"lane3intmapnum", 15, 14, 0, 11496, "Lane 3 internal mapped number"},

};
reg_decoder_t fecrs2_lanemapping_flds = {4, fecrs2_lanemapping_fld_list, 16};

reg_decoder_fld_t fecrs2_serlane0lo_fld_list[] = {
    {"serlane0",
     15,
     0,
     0,
     11497,
     "FEC symbol errors, FEC lanes 0 low Lower 16 bits"},

};
reg_decoder_t fecrs2_serlane0lo_flds = {1, fecrs2_serlane0lo_fld_list, 16};

reg_decoder_fld_t fecrs2_serlane0hi_fld_list[] = {
    {"serlane0",
     15,
     0,
     0,
     11498,
     "FEC symbol errors, FEC lanes 0 high Upper 16 bits"},

};
reg_decoder_t fecrs2_serlane0hi_flds = {1, fecrs2_serlane0hi_fld_list, 16};

reg_decoder_fld_t fecrs2_serlane1lo_fld_list[] = {
    {"serlane1",
     15,
     0,
     0,
     11499,
     "FEC symbol errors, FEC lanes 1low Lower 16 bits"},

};
reg_decoder_t fecrs2_serlane1lo_flds = {1, fecrs2_serlane1lo_fld_list, 16};

reg_decoder_fld_t fecrs2_serlane1hi_fld_list[] = {
    {"serlane1",
     15,
     0,
     0,
     11500,
     "FEC symbol errors, FEC lanes 1high Upper 16 bits"},

};
reg_decoder_t fecrs2_serlane1hi_flds = {1, fecrs2_serlane1hi_fld_list, 16};

reg_decoder_fld_t fecrs2_serlane2lo_fld_list[] = {
    {"serlane2",
     15,
     0,
     0,
     11501,
     "FEC symbol errors, FEC lanes 2low Lower 16 bits"},

};
reg_decoder_t fecrs2_serlane2lo_flds = {1, fecrs2_serlane2lo_fld_list, 16};

reg_decoder_fld_t fecrs2_serlane2hi_fld_list[] = {
    {"serlane2",
     15,
     0,
     0,
     11502,
     "FEC symbol errors, FEC lanes 2high Upper 16 bits"},

};
reg_decoder_t fecrs2_serlane2hi_flds = {1, fecrs2_serlane2hi_fld_list, 16};

reg_decoder_fld_t fecrs2_serlane3lo_fld_list[] = {
    {"serlane3",
     15,
     0,
     0,
     11503,
     "FEC symbol errors, FEC lanes 3low Lower 16 bits"},

};
reg_decoder_t fecrs2_serlane3lo_flds = {1, fecrs2_serlane3lo_fld_list, 16};

reg_decoder_fld_t fecrs2_serlane3hi_fld_list[] = {
    {"serlane3",
     15,
     0,
     0,
     11504,
     "FEC symbol errors, FEC lanes 3high Upper 16 bits"},

};
reg_decoder_t fecrs2_serlane3hi_flds = {1, fecrs2_serlane3hi_fld_list, 16};

reg_decoder_fld_t fecrs2_dbgctrl_fld_list[] = {
    {"softreset",
     0,
     0,
     0,
     11505,
     "Reset active high 1: reset 0: normal operation"},

    {"disablefec",
     1,
     1,
     0,
     11506,
     "1: disable FEC 0: enable FEC (normal operation)"},

    {"debuguseshortamp",
     2,
     2,
     0,
     11507,
     "Use short alignment marker. Internal use only."},

    {"debugswtestint",
     3,
     3,
     0,
     11508,
     "Software test interrupt. Internal use only."},

    {"debugcwteststoponfaildis", 4, 4, 0, 11509, "Internal use only. "},

    {"alwaysusecl49",
     6,
     6,
     0,
     11510,
     "When set to 1, always allow clause 49 block types."},

    {"neverusecl49",
     7,
     7,
     0,
     11511,
     "When set to 1, never allow clause 49 block types. It overrides "
     "alwaysusecl49."},

    {"pcsscrena",
     8,
     8,
     0,
     11512,
     "When set to 1, enables channelized pcs scrambler decoding."},

    {"mlgscrena",
     9,
     9,
     0,
     11513,
     "When set to 1, enables 100G aggragate mlg scrambler decoding. "},

};
reg_decoder_t fecrs2_dbgctrl_flds = {9, fecrs2_dbgctrl_fld_list, 16};

reg_decoder_fld_t fecrs2_dbgsts_fld_list[] = {
    {"Interrupt", 0, 0, 0, 11514, "Logical OR of all interrupts."},

    {"amplockstat",
     7,
     4,
     0,
     11515,
     "Alignment marker lock status per each lane"},

};
reg_decoder_t fecrs2_dbgsts_flds = {2, fecrs2_dbgsts_fld_list, 16};

reg_decoder_fld_t fecrs2_dbgberintthres_fld_list[] = {
    {"dbgberintthres",
     15,
     0,
     0,
     11516,
     "Specify BER Interrupt Threshold value"},

};
reg_decoder_t fecrs2_dbgberintthres_flds = {
    1, fecrs2_dbgberintthres_fld_list, 16};

reg_decoder_fld_t fecrs2_frcinvalidtimerlo_fld_list[] = {
    {"frcinvalidtimer",
     15,
     0,
     0,
     11517,
     "Amount of time FEC should force invalid Sync Headers when when "
     "correction_enable=1, bypass_indication=1 and hi_ser condition has been "
     "met (60-75ms)"},

};
reg_decoder_t fecrs2_frcinvalidtimerlo_flds = {
    1, fecrs2_frcinvalidtimerlo_fld_list, 16};

reg_decoder_fld_t fecrs2_frcinvalidtimerhi_fld_list[] = {
    {"frcinvalidtimer",
     15,
     0,
     0,
     11518,
     "Amount of time FEC should force invalid Sync Headers when when "
     "bypass_correction_enable=1, bypass_indication=1 and hi_ser condition has "
     "been met (60-75ms)"},

};
reg_decoder_t fecrs2_frcinvalidtimerhi_flds = {
    1, fecrs2_frcinvalidtimerhi_fld_list, 16};

reg_decoder_fld_t fecrs2_fint0_fld_list[] = {
    {"ovrdint", 6, 0, 0, 11519, "FEC AM lost lane 0 Interrupt override"},

};
reg_decoder_t fecrs2_fint0_flds = {1, fecrs2_fint0_fld_list, 16};

reg_decoder_fld_t fecrs2_spare0_fld_list[] = {
    {"spare0", 7, 0, 0, 11520, "Spare0"},

};
reg_decoder_t fecrs2_spare0_flds = {1, fecrs2_spare0_fld_list, 16};

reg_decoder_fld_t fecrs2_ieeecfg_fld_list[] = {
    {"blena74",
     0,
     0,
     0,
     11521,
     "Enable use of clause 74-style block lock without alignment markers (For "
     "IEEE draft compatibility)"},

    {"blrobust",
     3,
     1,
     0,
     11522,
     "Enables robust block lock. 3'b000 gains and loses lock based on good and "
     "bad codewords. 3'b111 gains and loses lcok based on correctable and "
     "uncorrectable codewords. 3'b011: waits for 1 good codeword, then uses "
     "correctable/uncorrectable. 3'b001 gains lock based .  on good codewords "
     "and loses lock based on uncorrectable codewords.  Only valid when Clause "
     "74 block lock is in use."},

    {"blgainth",
     5,
     4,
     0,
     11523,
     "Number of good/correctable consectutive blocks (-1) to see before "
     "locking.  Only valid when Clause 74 block lock is in use."},

    {"bllossth",
     8,
     6,
     0,
     11524,
     "Number of bad/uncorrectable consectutive blocks (-1) to see before "
     "losing lock. Only valid when Clause 74 block lock is in use."},

    {"padval",
     11,
     11,
     0,
     11525,
     "Pad value of 25G when transcoding alignment markers."},

    {"enascr", 12, 12, 0, 11526, "Enable Transcoding Scrambler"},

    {"enapn", 13, 13, 0, 11527, "Enable PN-2112/5280 Scrambling"},

    {"enaindi2",
     14,
     14,
     0,
     11528,
     "Enables sync header corruption on the 2nd 257-bit block on a "
     "uncorrectable codeword.  (For IEEE draft compatibility)"},

    {"enaindi6",
     15,
     15,
     0,
     11529,
     "Enables sync header corruption on the 6th 257-bit block on a "
     "uncorrectable codeword.  (For IEEE draft compatibility)"},

};
reg_decoder_t fecrs2_ieeecfg_flds = {9, fecrs2_ieeecfg_fld_list, 16};

reg_decoder_fld_t fecrs3_ctrl_fld_list[] = {
    {"bypcorrena",
     0,
     0,
     0,
     11530,
     "FEC bypass correction enable 1 = FEC decoder performs detection without "
     "correction 0 = FEC decoder performs detection and correction"},

    {"bypindiena",
     1,
     1,
     0,
     11531,
     "FEC bypass error indication 1 = FEC decoder does not indicate errors 0 = "
     "FEC decoder indicates errors to the PCS layer"},

};
reg_decoder_t fecrs3_ctrl_flds = {2, fecrs3_ctrl_fld_list, 16};

reg_decoder_fld_t fecrs3_sts_fld_list[] = {
    {"bypcorrabi",
     0,
     0,
     0,
     11532,
     "FEC bypass correction ability 1 = FEC decoder has ability to bypass "
     "error correction 0 = FEC decoder does not have correction bypass "
     "ability"},

    {"bypindiabi",
     1,
     1,
     0,
     11533,
     "FEC bypass indication ability 1 = FEC decoder has ability to bypass "
     "error indication to PCS 0 = FEC decoder always indicates errors to PCS"},

    {"hiser",
     2,
     2,
     0,
     11534,
     "hi_ser 1 = number of FEC symol errors over 8192 codewords exceeds "
     "threshold (KR4=417) 0 = number of FEC symol errors over 8192 codewords "
     "below threshold"},

    {"alignstatus",
     14,
     14,
     0,
     11535,
     "FEC align status 1: all lanes are synchronized and aligned 0: Deskew "
     "process not complete"},

};
reg_decoder_t fecrs3_sts_flds = {4, fecrs3_sts_fld_list, 16};

reg_decoder_fld_t fecrs3_corrcntlo_fld_list[] = {
    {"corrcnt",
     15,
     0,
     0,
     11536,
     "FEC_corrected_blocks_counter low (Lower 16 bits)"},

};
reg_decoder_t fecrs3_corrcntlo_flds = {1, fecrs3_corrcntlo_fld_list, 16};

reg_decoder_fld_t fecrs3_corrcnthi_fld_list[] = {
    {"corrcnt",
     15,
     0,
     0,
     11537,
     "FEC_corrected_blocks_counter High (Upper 16 bits)"},

};
reg_decoder_t fecrs3_corrcnthi_flds = {1, fecrs3_corrcnthi_fld_list, 16};

reg_decoder_fld_t fecrs3_uncorrcntlo_fld_list[] = {
    {"uncorrcnt",
     15,
     0,
     0,
     11538,
     " FEC_uncorrected_blocks_counter low (Lower 16 bits)"},

};
reg_decoder_t fecrs3_uncorrcntlo_flds = {1, fecrs3_uncorrcntlo_fld_list, 16};

reg_decoder_fld_t fecrs3_uncorrcnthi_fld_list[] = {
    {"uncorrcnt",
     15,
     0,
     0,
     11539,
     "FEC_uncorrected_blocks_counter High (Upper 16 bits)"},

};
reg_decoder_t fecrs3_uncorrcnthi_flds = {1, fecrs3_uncorrcnthi_fld_list, 16};

reg_decoder_fld_t fecrs3_lanemapping_fld_list[] = {
    {"lane0intmapnum", 9, 8, 0, 11540, "Lane 0 internal mapped number"},

    {"lane1intmapnum", 11, 10, 0, 11541, "Lane 1 internal mapped number"},

    {"lane2intmapnum", 13, 12, 0, 11542, "Lane 2 internal mapped number"},

    {"lane3intmapnum", 15, 14, 0, 11543, "Lane 3 internal mapped number"},

};
reg_decoder_t fecrs3_lanemapping_flds = {4, fecrs3_lanemapping_fld_list, 16};

reg_decoder_fld_t fecrs3_serlane0lo_fld_list[] = {
    {"serlane0",
     15,
     0,
     0,
     11544,
     "FEC symbol errors, FEC lanes 0 low Lower 16 bits"},

};
reg_decoder_t fecrs3_serlane0lo_flds = {1, fecrs3_serlane0lo_fld_list, 16};

reg_decoder_fld_t fecrs3_serlane0hi_fld_list[] = {
    {"serlane0",
     15,
     0,
     0,
     11545,
     "FEC symbol errors, FEC lanes 0 high Upper 16 bits"},

};
reg_decoder_t fecrs3_serlane0hi_flds = {1, fecrs3_serlane0hi_fld_list, 16};

reg_decoder_fld_t fecrs3_serlane1lo_fld_list[] = {
    {"serlane1",
     15,
     0,
     0,
     11546,
     "FEC symbol errors, FEC lanes 1low Lower 16 bits"},

};
reg_decoder_t fecrs3_serlane1lo_flds = {1, fecrs3_serlane1lo_fld_list, 16};

reg_decoder_fld_t fecrs3_serlane1hi_fld_list[] = {
    {"serlane1",
     15,
     0,
     0,
     11547,
     "FEC symbol errors, FEC lanes 1high Upper 16 bits"},

};
reg_decoder_t fecrs3_serlane1hi_flds = {1, fecrs3_serlane1hi_fld_list, 16};

reg_decoder_fld_t fecrs3_serlane2lo_fld_list[] = {
    {"serlane2",
     15,
     0,
     0,
     11548,
     "FEC symbol errors, FEC lanes 2low Lower 16 bits"},

};
reg_decoder_t fecrs3_serlane2lo_flds = {1, fecrs3_serlane2lo_fld_list, 16};

reg_decoder_fld_t fecrs3_serlane2hi_fld_list[] = {
    {"serlane2",
     15,
     0,
     0,
     11549,
     "FEC symbol errors, FEC lanes 2high Upper 16 bits"},

};
reg_decoder_t fecrs3_serlane2hi_flds = {1, fecrs3_serlane2hi_fld_list, 16};

reg_decoder_fld_t fecrs3_serlane3lo_fld_list[] = {
    {"serlane3",
     15,
     0,
     0,
     11550,
     "FEC symbol errors, FEC lanes 3low Lower 16 bits"},

};
reg_decoder_t fecrs3_serlane3lo_flds = {1, fecrs3_serlane3lo_fld_list, 16};

reg_decoder_fld_t fecrs3_serlane3hi_fld_list[] = {
    {"serlane3",
     15,
     0,
     0,
     11551,
     "FEC symbol errors, FEC lanes 3high Upper 16 bits"},

};
reg_decoder_t fecrs3_serlane3hi_flds = {1, fecrs3_serlane3hi_fld_list, 16};

reg_decoder_fld_t fecrs3_dbgctrl_fld_list[] = {
    {"softreset",
     0,
     0,
     0,
     11552,
     "Reset active high 1: reset 0: normal operation"},

    {"disablefec",
     1,
     1,
     0,
     11553,
     "1: disable FEC 0: enable FEC (normal operation)"},

    {"debuguseshortamp",
     2,
     2,
     0,
     11554,
     "Use short alignment marker. Internal use only."},

    {"debugswtestint",
     3,
     3,
     0,
     11555,
     "Software test interrupt. Internal use only."},

    {"debugcwteststoponfaildis", 4, 4, 0, 11556, "Internal use only. "},

    {"alwaysusecl49",
     6,
     6,
     0,
     11557,
     "When set to 1, always allow clause 49 block types."},

    {"neverusecl49",
     7,
     7,
     0,
     11558,
     "When set to 1, never allow clause 49 block types. It overrides "
     "alwaysusecl49."},

    {"pcsscrena",
     8,
     8,
     0,
     11559,
     "When set to 1, enables channelized pcs scrambler decoding."},

    {"mlgscrena",
     9,
     9,
     0,
     11560,
     "When set to 1, enables 100G aggragate mlg scrambler decoding. "},

};
reg_decoder_t fecrs3_dbgctrl_flds = {9, fecrs3_dbgctrl_fld_list, 16};

reg_decoder_fld_t fecrs3_dbgsts_fld_list[] = {
    {"Interrupt", 0, 0, 0, 11561, "Logical OR of all interrupts."},

    {"amplockstat",
     7,
     4,
     0,
     11562,
     "Alignment marker lock status per each lane"},

};
reg_decoder_t fecrs3_dbgsts_flds = {2, fecrs3_dbgsts_fld_list, 16};

reg_decoder_fld_t fecrs3_dbgberintthres_fld_list[] = {
    {"dbgberintthres",
     15,
     0,
     0,
     11563,
     "Specify BER Interrupt Threshold value"},

};
reg_decoder_t fecrs3_dbgberintthres_flds = {
    1, fecrs3_dbgberintthres_fld_list, 16};

reg_decoder_fld_t fecrs3_frcinvalidtimerlo_fld_list[] = {
    {"frcinvalidtimer",
     15,
     0,
     0,
     11564,
     "Amount of time FEC should force invalid Sync Headers when when "
     "correction_enable=1, bypass_indication=1 and hi_ser condition has been "
     "met (60-75ms)"},

};
reg_decoder_t fecrs3_frcinvalidtimerlo_flds = {
    1, fecrs3_frcinvalidtimerlo_fld_list, 16};

reg_decoder_fld_t fecrs3_frcinvalidtimerhi_fld_list[] = {
    {"frcinvalidtimer",
     15,
     0,
     0,
     11565,
     "Amount of time FEC should force invalid Sync Headers when when "
     "bypass_correction_enable=1, bypass_indication=1 and hi_ser condition has "
     "been met (60-75ms)"},

};
reg_decoder_t fecrs3_frcinvalidtimerhi_flds = {
    1, fecrs3_frcinvalidtimerhi_fld_list, 16};

reg_decoder_fld_t fecrs3_fint0_fld_list[] = {
    {"ovrdint", 6, 0, 0, 11566, "FEC AM lost lane 0 Interrupt override"},

};
reg_decoder_t fecrs3_fint0_flds = {1, fecrs3_fint0_fld_list, 16};

reg_decoder_fld_t fecrs3_spare0_fld_list[] = {
    {"spare0", 7, 0, 0, 11567, "Spare0"},

};
reg_decoder_t fecrs3_spare0_flds = {1, fecrs3_spare0_fld_list, 16};

reg_decoder_fld_t fecrs3_ieeecfg_fld_list[] = {
    {"blena74",
     0,
     0,
     0,
     11568,
     "Enable use of clause 74-style block lock without alignment markers (For "
     "IEEE draft compatibility)"},

    {"blrobust",
     3,
     1,
     0,
     11569,
     "Enables robust block lock. 3'b000 gains and loses lock based on good and "
     "bad codewords. 3'b111 gains and loses lcok based on correctable and "
     "uncorrectable codewords. 3'b011: waits for 1 good codeword, then uses "
     "correctable/uncorrectable. 3'b001 gains lock based .  on good codewords "
     "and loses lock based on uncorrectable codewords.  Only valid when Clause "
     "74 block lock is in use."},

    {"blgainth",
     5,
     4,
     0,
     11570,
     "Number of good/correctable consectutive blocks (-1) to see before "
     "locking.  Only valid when Clause 74 block lock is in use."},

    {"bllossth",
     8,
     6,
     0,
     11571,
     "Number of bad/uncorrectable consectutive blocks (-1) to see before "
     "losing lock. Only valid when Clause 74 block lock is in use."},

    {"padval",
     11,
     11,
     0,
     11572,
     "Pad value of 25G when transcoding alignment markers."},

    {"enascr", 12, 12, 0, 11573, "Enable Transcoding Scrambler"},

    {"enapn", 13, 13, 0, 11574, "Enable PN-2112/5280 Scrambling"},

    {"enaindi2",
     14,
     14,
     0,
     11575,
     "Enables sync header corruption on the 2nd 257-bit block on a "
     "uncorrectable codeword.  (For IEEE draft compatibility)"},

    {"enaindi6",
     15,
     15,
     0,
     11576,
     "Enables sync header corruption on the 6th 257-bit block on a "
     "uncorrectable codeword.  (For IEEE draft compatibility)"},

};
reg_decoder_t fecrs3_ieeecfg_flds = {9, fecrs3_ieeecfg_fld_list, 16};

reg_decoder_fld_t fecfc0_fint0_fld_list[] = {
    {"ovrdint", 3, 0, 0, 11577, "Block lock gained Interrupt override"},

    {"ovrdint1", 8, 5, 0, 11578, "Block lock gained Interrupt override"},

};
reg_decoder_t fecfc0_fint0_flds = {2, fecfc0_fint0_fld_list, 16};

reg_decoder_fld_t fecfc0_sts_fld_list[] = {
    {"fcblocklock", 0, 0, 0, 11579, "Block lock status indication."},

    {"fcability",
     1,
     1,
     0,
     11580,
     "The FC FEC Ability field is high if a FC FEC resource is available (or "
     "if the FC FEC is enabled for that channel). The only time this bit will "
     "be low is when all FC FEC resources are used (only possible in CFG1 "
     "where the number of channels exceeds the number of FC FEC resources)"},

    {"ena",
     2,
     2,
     0,
     11581,
     "Indicated if FC FEC is active on current channel."},

};
reg_decoder_t fecfc0_sts_flds = {3, fecfc0_sts_fld_list, 16};

reg_decoder_fld_t fecfc0_cfg_fld_list[] = {
    {"enareq",
     0,
     0,
     0,
     11582,
     "When fec is disabled, internal bypass for rx and tx are activated."},

    {"enapcserr",
     1,
     1,
     0,
     11583,
     "When the variable is set to one, this enables indication of decoding "
     "errors through the sync bits to the PCS layer  as defined in 802.3-2012 "
     "Clause 74.7.4.5. When set to zero, the error indication function is "
     "disabled."},

    {"enaerrcorr",
     2,
     2,
     0,
     11584,
     "Enable error correction, when disabled (0) any block with a bad parity "
     "is treated as uncorrectable."},

};
reg_decoder_t fecfc0_cfg_flds = {3, fecfc0_cfg_fld_list, 16};

reg_decoder_fld_t fecfc0_corrblockscounterhi_fld_list[] = {
    {"corrblockscounter",
     15,
     0,
     0,
     11585,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc0_corrblockscounterhi_flds = {
    1, fecfc0_corrblockscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc0_corrblockscounterlo_fld_list[] = {
    {"corrblockscounter",
     15,
     0,
     0,
     11586,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc0_corrblockscounterlo_flds = {
    1, fecfc0_corrblockscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc0_uncorrblockscounterhi_fld_list[] = {
    {"uncorrblockscounter",
     15,
     0,
     0,
     11587,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc0_uncorrblockscounterhi_flds = {
    1, fecfc0_uncorrblockscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc0_uncorrblockscounterlo_fld_list[] = {
    {"uncorrblockscounter",
     15,
     0,
     0,
     11588,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc0_uncorrblockscounterlo_flds = {
    1, fecfc0_uncorrblockscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc0_stsvl1_fld_list[] = {
    {"fcblocklock", 0, 0, 0, 11589, "Block lock status indication."},

};
reg_decoder_t fecfc0_stsvl1_flds = {1, fecfc0_stsvl1_fld_list, 16};

reg_decoder_fld_t fecfc0_corrblockscounterhivl1_fld_list[] = {
    {"corrblockscountervl1",
     15,
     0,
     0,
     11590,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc0_corrblockscounterhivl1_flds = {
    1, fecfc0_corrblockscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc0_corrblockscounterlovl1_fld_list[] = {
    {"corrblockscountervl1",
     15,
     0,
     0,
     11591,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc0_corrblockscounterlovl1_flds = {
    1, fecfc0_corrblockscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc0_uncorrblockscounterhivl1_fld_list[] = {
    {"uncorrblockscountervl1",
     15,
     0,
     0,
     11592,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc0_uncorrblockscounterhivl1_flds = {
    1, fecfc0_uncorrblockscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc0_uncorrblockscounterlovl1_fld_list[] = {
    {"uncorrblockscountervl1",
     15,
     0,
     0,
     11593,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc0_uncorrblockscounterlovl1_flds = {
    1, fecfc0_uncorrblockscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc0_dbgvl1_fld_list[] = {
    {"forceblocklock",
     3,
     3,
     0,
     11594,
     "Causes the block lock state to be forced to true. (manual override) "
     "Debug purposes only."},

    {"forcebitslip",
     4,
     4,
     0,
     11595,
     "Causes the current block position to slip by one bit, (manual override) "
     "cleared by hardware when bit slip operation is complete. Debug purposes "
     "only."},

};
reg_decoder_t fecfc0_dbgvl1_flds = {2, fecfc0_dbgvl1_fld_list, 16};

reg_decoder_fld_t fecfc0_dbgcorrbitscounterhivl1_fld_list[] = {
    {"corrbitscountervl1",
     15,
     0,
     0,
     11596,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc0_dbgcorrbitscounterhivl1_flds = {
    1, fecfc0_dbgcorrbitscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc0_dbgcorrbitscounterlovl1_fld_list[] = {
    {"corrbitscountervl1",
     15,
     0,
     0,
     11597,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc0_dbgcorrbitscounterlovl1_flds = {
    1, fecfc0_dbgcorrbitscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc0_dbgblockcounthivl1_fld_list[] = {
    {"blockcountvl1",
     15,
     0,
     0,
     11598,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc0_dbgblockcounthivl1_flds = {
    1, fecfc0_dbgblockcounthivl1_fld_list, 16};

reg_decoder_fld_t fecfc0_dbgblockcountlovl1_fld_list[] = {
    {"blockcountvl1",
     15,
     0,
     0,
     11599,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc0_dbgblockcountlovl1_flds = {
    1, fecfc0_dbgblockcountlovl1_fld_list, 16};

reg_decoder_fld_t fecfc0_dbg_fld_list[] = {
    {"bypscram",
     0,
     0,
     0,
     11600,
     "Bypasses psuedo-noise generation on RX and TX."},

    {"reset",
     1,
     1,
     0,
     11601,
     "Write to 1 force FEC into reset. Clear to 0 to release reset. Debug "
     "purposes only. Will corrupt any passing traffic."},

    {"enarobblocklock",
     2,
     2,
     0,
     11602,
     "Allows the block lock FSM to use corrected codewords as good code words. "
     "(allows link to persit with continuous correctable errors)"},

    {"forceblocklock",
     3,
     3,
     0,
     11603,
     "Causes the block lock state to be forced to true. (manual override) "
     "Debug purposes only."},

    {"forcebitslip",
     4,
     4,
     0,
     11604,
     "Causes the current block position to slip by one bit, (manual override) "
     "cleared by hardware when bit slip operation is complete. Debug purposes "
     "only."},

};
reg_decoder_t fecfc0_dbg_flds = {5, fecfc0_dbg_fld_list, 16};

reg_decoder_fld_t fecfc0_dbgcorrbitscounterhi_fld_list[] = {
    {"corrbitscounter",
     15,
     0,
     0,
     11605,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc0_dbgcorrbitscounterhi_flds = {
    1, fecfc0_dbgcorrbitscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc0_dbgcorrbitscounterlo_fld_list[] = {
    {"corrbitscounter",
     15,
     0,
     0,
     11606,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc0_dbgcorrbitscounterlo_flds = {
    1, fecfc0_dbgcorrbitscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc0_dbgblockcounthi_fld_list[] = {
    {"blockcount",
     15,
     0,
     0,
     11607,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc0_dbgblockcounthi_flds = {
    1, fecfc0_dbgblockcounthi_fld_list, 16};

reg_decoder_fld_t fecfc0_dbgblockcountlo_fld_list[] = {
    {"blockcount",
     15,
     0,
     0,
     11608,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc0_dbgblockcountlo_flds = {
    1, fecfc0_dbgblockcountlo_fld_list, 16};

reg_decoder_fld_t fecfc0_spare0_fld_list[] = {
    {"spare0", 7, 0, 0, 11609, "Spare0"},

};
reg_decoder_t fecfc0_spare0_flds = {1, fecfc0_spare0_fld_list, 16};

reg_decoder_fld_t fecfc1_fint0_fld_list[] = {
    {"ovrdint", 3, 0, 0, 11610, "Block lock gained Interrupt override"},

    {"ovrdint1", 8, 5, 0, 11611, "Block lock gained Interrupt override"},

};
reg_decoder_t fecfc1_fint0_flds = {2, fecfc1_fint0_fld_list, 16};

reg_decoder_fld_t fecfc1_sts_fld_list[] = {
    {"fcblocklock", 0, 0, 0, 11612, "Block lock status indication."},

    {"fcability",
     1,
     1,
     0,
     11613,
     "The FC FEC Ability field is high if a FC FEC resource is available (or "
     "if the FC FEC is enabled for that channel). The only time this bit will "
     "be low is when all FC FEC resources are used (only possible in CFG1 "
     "where the number of channels exceeds the number of FC FEC resources)"},

    {"ena",
     2,
     2,
     0,
     11614,
     "Indicated if FC FEC is active on current channel."},

};
reg_decoder_t fecfc1_sts_flds = {3, fecfc1_sts_fld_list, 16};

reg_decoder_fld_t fecfc1_cfg_fld_list[] = {
    {"enareq",
     0,
     0,
     0,
     11615,
     "When fec is disabled, internal bypass for rx and tx are activated."},

    {"enapcserr",
     1,
     1,
     0,
     11616,
     "When the variable is set to one, this enables indication of decoding "
     "errors through the sync bits to the PCS layer  as defined in 802.3-2012 "
     "Clause 74.7.4.5. When set to zero, the error indication function is "
     "disabled."},

    {"enaerrcorr",
     2,
     2,
     0,
     11617,
     "Enable error correction, when disabled (0) any block with a bad parity "
     "is treated as uncorrectable."},

};
reg_decoder_t fecfc1_cfg_flds = {3, fecfc1_cfg_fld_list, 16};

reg_decoder_fld_t fecfc1_corrblockscounterhi_fld_list[] = {
    {"corrblockscounter",
     15,
     0,
     0,
     11618,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc1_corrblockscounterhi_flds = {
    1, fecfc1_corrblockscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc1_corrblockscounterlo_fld_list[] = {
    {"corrblockscounter",
     15,
     0,
     0,
     11619,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc1_corrblockscounterlo_flds = {
    1, fecfc1_corrblockscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc1_uncorrblockscounterhi_fld_list[] = {
    {"uncorrblockscounter",
     15,
     0,
     0,
     11620,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc1_uncorrblockscounterhi_flds = {
    1, fecfc1_uncorrblockscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc1_uncorrblockscounterlo_fld_list[] = {
    {"uncorrblockscounter",
     15,
     0,
     0,
     11621,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc1_uncorrblockscounterlo_flds = {
    1, fecfc1_uncorrblockscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc1_stsvl1_fld_list[] = {
    {"fcblocklock", 0, 0, 0, 11622, "Block lock status indication."},

};
reg_decoder_t fecfc1_stsvl1_flds = {1, fecfc1_stsvl1_fld_list, 16};

reg_decoder_fld_t fecfc1_corrblockscounterhivl1_fld_list[] = {
    {"corrblockscountervl1",
     15,
     0,
     0,
     11623,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc1_corrblockscounterhivl1_flds = {
    1, fecfc1_corrblockscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc1_corrblockscounterlovl1_fld_list[] = {
    {"corrblockscountervl1",
     15,
     0,
     0,
     11624,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc1_corrblockscounterlovl1_flds = {
    1, fecfc1_corrblockscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc1_uncorrblockscounterhivl1_fld_list[] = {
    {"uncorrblockscountervl1",
     15,
     0,
     0,
     11625,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc1_uncorrblockscounterhivl1_flds = {
    1, fecfc1_uncorrblockscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc1_uncorrblockscounterlovl1_fld_list[] = {
    {"uncorrblockscountervl1",
     15,
     0,
     0,
     11626,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc1_uncorrblockscounterlovl1_flds = {
    1, fecfc1_uncorrblockscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc1_dbgvl1_fld_list[] = {
    {"forceblocklock",
     3,
     3,
     0,
     11627,
     "Causes the block lock state to be forced to true. (manual override) "
     "Debug purposes only."},

    {"forcebitslip",
     4,
     4,
     0,
     11628,
     "Causes the current block position to slip by one bit, (manual override) "
     "cleared by hardware when bit slip operation is complete. Debug purposes "
     "only."},

};
reg_decoder_t fecfc1_dbgvl1_flds = {2, fecfc1_dbgvl1_fld_list, 16};

reg_decoder_fld_t fecfc1_dbgcorrbitscounterhivl1_fld_list[] = {
    {"corrbitscountervl1",
     15,
     0,
     0,
     11629,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc1_dbgcorrbitscounterhivl1_flds = {
    1, fecfc1_dbgcorrbitscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc1_dbgcorrbitscounterlovl1_fld_list[] = {
    {"corrbitscountervl1",
     15,
     0,
     0,
     11630,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc1_dbgcorrbitscounterlovl1_flds = {
    1, fecfc1_dbgcorrbitscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc1_dbgblockcounthivl1_fld_list[] = {
    {"blockcountvl1",
     15,
     0,
     0,
     11631,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc1_dbgblockcounthivl1_flds = {
    1, fecfc1_dbgblockcounthivl1_fld_list, 16};

reg_decoder_fld_t fecfc1_dbgblockcountlovl1_fld_list[] = {
    {"blockcountvl1",
     15,
     0,
     0,
     11632,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc1_dbgblockcountlovl1_flds = {
    1, fecfc1_dbgblockcountlovl1_fld_list, 16};

reg_decoder_fld_t fecfc1_dbg_fld_list[] = {
    {"bypscram",
     0,
     0,
     0,
     11633,
     "Bypasses psuedo-noise generation on RX and TX."},

    {"reset",
     1,
     1,
     0,
     11634,
     "Write to 1 force FEC into reset. Clear to 0 to release reset. Debug "
     "purposes only. Will corrupt any passing traffic."},

    {"enarobblocklock",
     2,
     2,
     0,
     11635,
     "Allows the block lock FSM to use corrected codewords as good code words. "
     "(allows link to persit with continuous correctable errors)"},

    {"forceblocklock",
     3,
     3,
     0,
     11636,
     "Causes the block lock state to be forced to true. (manual override) "
     "Debug purposes only."},

    {"forcebitslip",
     4,
     4,
     0,
     11637,
     "Causes the current block position to slip by one bit, (manual override) "
     "cleared by hardware when bit slip operation is complete. Debug purposes "
     "only."},

};
reg_decoder_t fecfc1_dbg_flds = {5, fecfc1_dbg_fld_list, 16};

reg_decoder_fld_t fecfc1_dbgcorrbitscounterhi_fld_list[] = {
    {"corrbitscounter",
     15,
     0,
     0,
     11638,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc1_dbgcorrbitscounterhi_flds = {
    1, fecfc1_dbgcorrbitscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc1_dbgcorrbitscounterlo_fld_list[] = {
    {"corrbitscounter",
     15,
     0,
     0,
     11639,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc1_dbgcorrbitscounterlo_flds = {
    1, fecfc1_dbgcorrbitscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc1_dbgblockcounthi_fld_list[] = {
    {"blockcount",
     15,
     0,
     0,
     11640,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc1_dbgblockcounthi_flds = {
    1, fecfc1_dbgblockcounthi_fld_list, 16};

reg_decoder_fld_t fecfc1_dbgblockcountlo_fld_list[] = {
    {"blockcount",
     15,
     0,
     0,
     11641,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc1_dbgblockcountlo_flds = {
    1, fecfc1_dbgblockcountlo_fld_list, 16};

reg_decoder_fld_t fecfc2_fint0_fld_list[] = {
    {"ovrdint", 3, 0, 0, 11642, "Block lock gained Interrupt override"},

    {"ovrdint1", 8, 5, 0, 11643, "Block lock gained Interrupt override"},

};
reg_decoder_t fecfc2_fint0_flds = {2, fecfc2_fint0_fld_list, 16};

reg_decoder_fld_t fecfc2_sts_fld_list[] = {
    {"fcblocklock", 0, 0, 0, 11644, "Block lock status indication."},

    {"fcability",
     1,
     1,
     0,
     11645,
     "The FC FEC Ability field is high if a FC FEC resource is available (or "
     "if the FC FEC is enabled for that channel). The only time this bit will "
     "be low is when all FC FEC resources are used (only possible in CFG1 "
     "where the number of channels exceeds the number of FC FEC resources)"},

    {"ena",
     2,
     2,
     0,
     11646,
     "Indicated if FC FEC is active on current channel."},

};
reg_decoder_t fecfc2_sts_flds = {3, fecfc2_sts_fld_list, 16};

reg_decoder_fld_t fecfc2_cfg_fld_list[] = {
    {"enareq",
     0,
     0,
     0,
     11647,
     "When fec is disabled, internal bypass for rx and tx are activated."},

    {"enapcserr",
     1,
     1,
     0,
     11648,
     "When the variable is set to one, this enables indication of decoding "
     "errors through the sync bits to the PCS layer  as defined in 802.3-2012 "
     "Clause 74.7.4.5. When set to zero, the error indication function is "
     "disabled."},

    {"enaerrcorr",
     2,
     2,
     0,
     11649,
     "Enable error correction, when disabled (0) any block with a bad parity "
     "is treated as uncorrectable."},

};
reg_decoder_t fecfc2_cfg_flds = {3, fecfc2_cfg_fld_list, 16};

reg_decoder_fld_t fecfc2_corrblockscounterhi_fld_list[] = {
    {"corrblockscounter",
     15,
     0,
     0,
     11650,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc2_corrblockscounterhi_flds = {
    1, fecfc2_corrblockscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc2_corrblockscounterlo_fld_list[] = {
    {"corrblockscounter",
     15,
     0,
     0,
     11651,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc2_corrblockscounterlo_flds = {
    1, fecfc2_corrblockscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc2_uncorrblockscounterhi_fld_list[] = {
    {"uncorrblockscounter",
     15,
     0,
     0,
     11652,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc2_uncorrblockscounterhi_flds = {
    1, fecfc2_uncorrblockscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc2_uncorrblockscounterlo_fld_list[] = {
    {"uncorrblockscounter",
     15,
     0,
     0,
     11653,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc2_uncorrblockscounterlo_flds = {
    1, fecfc2_uncorrblockscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc2_stsvl1_fld_list[] = {
    {"fcblocklock", 0, 0, 0, 11654, "Block lock status indication."},

};
reg_decoder_t fecfc2_stsvl1_flds = {1, fecfc2_stsvl1_fld_list, 16};

reg_decoder_fld_t fecfc2_corrblockscounterhivl1_fld_list[] = {
    {"corrblockscountervl1",
     15,
     0,
     0,
     11655,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc2_corrblockscounterhivl1_flds = {
    1, fecfc2_corrblockscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc2_corrblockscounterlovl1_fld_list[] = {
    {"corrblockscountervl1",
     15,
     0,
     0,
     11656,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc2_corrblockscounterlovl1_flds = {
    1, fecfc2_corrblockscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc2_uncorrblockscounterhivl1_fld_list[] = {
    {"uncorrblockscountervl1",
     15,
     0,
     0,
     11657,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc2_uncorrblockscounterhivl1_flds = {
    1, fecfc2_uncorrblockscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc2_uncorrblockscounterlovl1_fld_list[] = {
    {"uncorrblockscountervl1",
     15,
     0,
     0,
     11658,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc2_uncorrblockscounterlovl1_flds = {
    1, fecfc2_uncorrblockscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc2_dbgvl1_fld_list[] = {
    {"forceblocklock",
     3,
     3,
     0,
     11659,
     "Causes the block lock state to be forced to true. (manual override) "
     "Debug purposes only."},

    {"forcebitslip",
     4,
     4,
     0,
     11660,
     "Causes the current block position to slip by one bit, (manual override) "
     "cleared by hardware when bit slip operation is complete. Debug purposes "
     "only."},

};
reg_decoder_t fecfc2_dbgvl1_flds = {2, fecfc2_dbgvl1_fld_list, 16};

reg_decoder_fld_t fecfc2_dbgcorrbitscounterhivl1_fld_list[] = {
    {"corrbitscountervl1",
     15,
     0,
     0,
     11661,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc2_dbgcorrbitscounterhivl1_flds = {
    1, fecfc2_dbgcorrbitscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc2_dbgcorrbitscounterlovl1_fld_list[] = {
    {"corrbitscountervl1",
     15,
     0,
     0,
     11662,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc2_dbgcorrbitscounterlovl1_flds = {
    1, fecfc2_dbgcorrbitscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc2_dbgblockcounthivl1_fld_list[] = {
    {"blockcountvl1",
     15,
     0,
     0,
     11663,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc2_dbgblockcounthivl1_flds = {
    1, fecfc2_dbgblockcounthivl1_fld_list, 16};

reg_decoder_fld_t fecfc2_dbgblockcountlovl1_fld_list[] = {
    {"blockcountvl1",
     15,
     0,
     0,
     11664,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc2_dbgblockcountlovl1_flds = {
    1, fecfc2_dbgblockcountlovl1_fld_list, 16};

reg_decoder_fld_t fecfc2_dbg_fld_list[] = {
    {"bypscram",
     0,
     0,
     0,
     11665,
     "Bypasses psuedo-noise generation on RX and TX."},

    {"reset",
     1,
     1,
     0,
     11666,
     "Write to 1 force FEC into reset. Clear to 0 to release reset. Debug "
     "purposes only. Will corrupt any passing traffic."},

    {"enarobblocklock",
     2,
     2,
     0,
     11667,
     "Allows the block lock FSM to use corrected codewords as good code words. "
     "(allows link to persit with continuous correctable errors)"},

    {"forceblocklock",
     3,
     3,
     0,
     11668,
     "Causes the block lock state to be forced to true. (manual override) "
     "Debug purposes only."},

    {"forcebitslip",
     4,
     4,
     0,
     11669,
     "Causes the current block position to slip by one bit, (manual override) "
     "cleared by hardware when bit slip operation is complete. Debug purposes "
     "only."},

};
reg_decoder_t fecfc2_dbg_flds = {5, fecfc2_dbg_fld_list, 16};

reg_decoder_fld_t fecfc2_dbgcorrbitscounterhi_fld_list[] = {
    {"corrbitscounter",
     15,
     0,
     0,
     11670,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc2_dbgcorrbitscounterhi_flds = {
    1, fecfc2_dbgcorrbitscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc2_dbgcorrbitscounterlo_fld_list[] = {
    {"corrbitscounter",
     15,
     0,
     0,
     11671,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc2_dbgcorrbitscounterlo_flds = {
    1, fecfc2_dbgcorrbitscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc2_dbgblockcounthi_fld_list[] = {
    {"blockcount",
     15,
     0,
     0,
     11672,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc2_dbgblockcounthi_flds = {
    1, fecfc2_dbgblockcounthi_fld_list, 16};

reg_decoder_fld_t fecfc2_dbgblockcountlo_fld_list[] = {
    {"blockcount",
     15,
     0,
     0,
     11673,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc2_dbgblockcountlo_flds = {
    1, fecfc2_dbgblockcountlo_fld_list, 16};

reg_decoder_fld_t fecfc3_fint0_fld_list[] = {
    {"ovrdint", 3, 0, 0, 11674, "Block lock gained Interrupt override"},

    {"ovrdint1", 8, 5, 0, 11675, "Block lock gained Interrupt override"},

};
reg_decoder_t fecfc3_fint0_flds = {2, fecfc3_fint0_fld_list, 16};

reg_decoder_fld_t fecfc3_sts_fld_list[] = {
    {"fcblocklock", 0, 0, 0, 11676, "Block lock status indication."},

    {"fcability",
     1,
     1,
     0,
     11677,
     "The FC FEC Ability field is high if a FC FEC resource is available (or "
     "if the FC FEC is enabled for that channel). The only time this bit will "
     "be low is when all FC FEC resources are used (only possible in CFG1 "
     "where the number of channels exceeds the number of FC FEC resources)"},

    {"ena",
     2,
     2,
     0,
     11678,
     "Indicated if FC FEC is active on current channel."},

};
reg_decoder_t fecfc3_sts_flds = {3, fecfc3_sts_fld_list, 16};

reg_decoder_fld_t fecfc3_cfg_fld_list[] = {
    {"enareq",
     0,
     0,
     0,
     11679,
     "When fec is disabled, internal bypass for rx and tx are activated."},

    {"enapcserr",
     1,
     1,
     0,
     11680,
     "When the variable is set to one, this enables indication of decoding "
     "errors through the sync bits to the PCS layer  as defined in 802.3-2012 "
     "Clause 74.7.4.5. When set to zero, the error indication function is "
     "disabled."},

    {"enaerrcorr",
     2,
     2,
     0,
     11681,
     "Enable error correction, when disabled (0) any block with a bad parity "
     "is treated as uncorrectable."},

};
reg_decoder_t fecfc3_cfg_flds = {3, fecfc3_cfg_fld_list, 16};

reg_decoder_fld_t fecfc3_corrblockscounterhi_fld_list[] = {
    {"corrblockscounter",
     15,
     0,
     0,
     11682,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc3_corrblockscounterhi_flds = {
    1, fecfc3_corrblockscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc3_corrblockscounterlo_fld_list[] = {
    {"corrblockscounter",
     15,
     0,
     0,
     11683,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc3_corrblockscounterlo_flds = {
    1, fecfc3_corrblockscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc3_uncorrblockscounterhi_fld_list[] = {
    {"uncorrblockscounter",
     15,
     0,
     0,
     11684,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc3_uncorrblockscounterhi_flds = {
    1, fecfc3_uncorrblockscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc3_uncorrblockscounterlo_fld_list[] = {
    {"uncorrblockscounter",
     15,
     0,
     0,
     11685,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc3_uncorrblockscounterlo_flds = {
    1, fecfc3_uncorrblockscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc3_stsvl1_fld_list[] = {
    {"fcblocklock", 0, 0, 0, 11686, "Block lock status indication."},

};
reg_decoder_t fecfc3_stsvl1_flds = {1, fecfc3_stsvl1_fld_list, 16};

reg_decoder_fld_t fecfc3_corrblockscounterhivl1_fld_list[] = {
    {"corrblockscountervl1",
     15,
     0,
     0,
     11687,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc3_corrblockscounterhivl1_flds = {
    1, fecfc3_corrblockscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc3_corrblockscounterlovl1_fld_list[] = {
    {"corrblockscountervl1",
     15,
     0,
     0,
     11688,
     "A corrected block is an FEC block that has invalid parity, and has been "
     "corrected by the FEC decoder. This counter clears on read and saturates "
     "on overflow."},

};
reg_decoder_t fecfc3_corrblockscounterlovl1_flds = {
    1, fecfc3_corrblockscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc3_uncorrblockscounterhivl1_fld_list[] = {
    {"uncorrblockscountervl1",
     15,
     0,
     0,
     11689,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc3_uncorrblockscounterhivl1_flds = {
    1, fecfc3_uncorrblockscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc3_uncorrblockscounterlovl1_fld_list[] = {
    {"uncorrblockscountervl1",
     15,
     0,
     0,
     11690,
     "An uncorrected block is an FEC block that has invalid parity, and has "
     "not been corrected by the FEC decoder. This counter clears on read and "
     "saturates on overflow."},

};
reg_decoder_t fecfc3_uncorrblockscounterlovl1_flds = {
    1, fecfc3_uncorrblockscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc3_dbgvl1_fld_list[] = {
    {"forceblocklock",
     3,
     3,
     0,
     11691,
     "Causes the block lock state to be forced to true. (manual override) "
     "Debug purposes only."},

    {"forcebitslip",
     4,
     4,
     0,
     11692,
     "Causes the current block position to slip by one bit, (manual override) "
     "cleared by hardware when bit slip operation is complete. Debug purposes "
     "only."},

};
reg_decoder_t fecfc3_dbgvl1_flds = {2, fecfc3_dbgvl1_fld_list, 16};

reg_decoder_fld_t fecfc3_dbgcorrbitscounterhivl1_fld_list[] = {
    {"corrbitscountervl1",
     15,
     0,
     0,
     11693,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc3_dbgcorrbitscounterhivl1_flds = {
    1, fecfc3_dbgcorrbitscounterhivl1_fld_list, 16};

reg_decoder_fld_t fecfc3_dbgcorrbitscounterlovl1_fld_list[] = {
    {"corrbitscountervl1",
     15,
     0,
     0,
     11694,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc3_dbgcorrbitscounterlovl1_flds = {
    1, fecfc3_dbgcorrbitscounterlovl1_fld_list, 16};

reg_decoder_fld_t fecfc3_dbgblockcounthivl1_fld_list[] = {
    {"blockcountvl1",
     15,
     0,
     0,
     11695,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc3_dbgblockcounthivl1_flds = {
    1, fecfc3_dbgblockcounthivl1_fld_list, 16};

reg_decoder_fld_t fecfc3_dbgblockcountlovl1_fld_list[] = {
    {"blockcountvl1",
     15,
     0,
     0,
     11696,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc3_dbgblockcountlovl1_flds = {
    1, fecfc3_dbgblockcountlovl1_fld_list, 16};

reg_decoder_fld_t fecfc3_dbg_fld_list[] = {
    {"bypscram",
     0,
     0,
     0,
     11697,
     "Bypasses psuedo-noise generation on RX and TX."},

    {"reset",
     1,
     1,
     0,
     11698,
     "Write to 1 force FEC into reset. Clear to 0 to release reset. Debug "
     "purposes only. Will corrupt any passing traffic."},

    {"enarobblocklock",
     2,
     2,
     0,
     11699,
     "Allows the block lock FSM to use corrected codewords as good code words. "
     "(allows link to persit with continuous correctable errors)"},

    {"forceblocklock",
     3,
     3,
     0,
     11700,
     "Causes the block lock state to be forced to true. (manual override) "
     "Debug purposes only."},

    {"forcebitslip",
     4,
     4,
     0,
     11701,
     "Causes the current block position to slip by one bit, (manual override) "
     "cleared by hardware when bit slip operation is complete. Debug purposes "
     "only."},

};
reg_decoder_t fecfc3_dbg_flds = {5, fecfc3_dbg_fld_list, 16};

reg_decoder_fld_t fecfc3_dbgcorrbitscounterhi_fld_list[] = {
    {"corrbitscounter",
     15,
     0,
     0,
     11702,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc3_dbgcorrbitscounterhi_flds = {
    1, fecfc3_dbgcorrbitscounterhi_fld_list, 16};

reg_decoder_fld_t fecfc3_dbgcorrbitscounterlo_fld_list[] = {
    {"corrbitscounter",
     15,
     0,
     0,
     11703,
     "Counts bits corrected by FEC since last read.  This counter clears on "
     "read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc3_dbgcorrbitscounterlo_flds = {
    1, fecfc3_dbgcorrbitscounterlo_fld_list, 16};

reg_decoder_fld_t fecfc3_dbgblockcounthi_fld_list[] = {
    {"blockcount",
     15,
     0,
     0,
     11704,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc3_dbgblockcounthi_flds = {
    1, fecfc3_dbgblockcounthi_fld_list, 16};

reg_decoder_fld_t fecfc3_dbgblockcountlo_fld_list[] = {
    {"blockcount",
     15,
     0,
     0,
     11705,
     "Counts 2112 bit blocks seen on FEC since last read.  This counter clears "
     "on read and saturates on overflow. (part of BER GAIN measurement)"},

};
reg_decoder_t fecfc3_dbgblockcountlo_flds = {
    1, fecfc3_dbgblockcountlo_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_ctrl_fld_list[] = {
    {"anrestart", 1, 1, 0, 11706, "1: restart Auto Negotiation"},

    {"anenable",
     0,
     0,
     0,
     11707,
     "1: Enable Auto Negotiation 0: Disable Auto Negotiation"},

};
reg_decoder_t lsmcpcs0_ctrl_flds = {2, lsmcpcs0_ctrl_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_mradvability_fld_list[] = {
    {"basepg15",
     15,
     15,
     0,
     11708,
     "A register that contains the advertised ability base page of the local "
     "device to be conveyed to tx_config_reg<D15> for transmission to the link "
     "partner."},

    {"basepg",
     13,
     0,
     0,
     11709,
     "A 14-bit register that contains the advertised ability base page of the "
     "local device to be conveyed to tx_config_reg<D13:D0> for transmission to "
     "the link partner."},

};
reg_decoder_t lsmcpcs0_mradvability_flds = {
    2, lsmcpcs0_mradvability_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_rxconfig_fld_list[] = {
    {"rxconfig", 15, 0, 0, 11710, "RX AN config"},

};
reg_decoder_t lsmcpcs0_rxconfig_flds = {1, lsmcpcs0_rxconfig_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_lnktimer_fld_list[] = {
    {"lnktimer", 15, 0, 0, 11711, "default set to 10 ms for 125 MHz gb clk"},

};
reg_decoder_t lsmcpcs0_lnktimer_flds = {1, lsmcpcs0_lnktimer_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_lnktimer2_fld_list[] = {
    {"lnktimer", 7, 0, 0, 11712, "default set to 10 ms for 125 MHz gb clk"},

};
reg_decoder_t lsmcpcs0_lnktimer2_flds = {1, lsmcpcs0_lnktimer2_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_disperrcnt_fld_list[] = {
    {"disperrcnt",
     15,
     0,
     0,
     11713,
     "Disparity error count for channel N. Clears on read."},

};
reg_decoder_t lsmcpcs0_disperrcnt_flds = {1, lsmcpcs0_disperrcnt_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_invcodecnt_fld_list[] = {
    {"invcodecnt",
     15,
     0,
     0,
     11714,
     "Invalid code count for channel N. Clears on read."},

};
reg_decoder_t lsmcpcs0_invcodecnt_flds = {1, lsmcpcs0_invcodecnt_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_sts_fld_list[] = {
    {"syncstat", 1, 1, 0, 11715, "1: OK 0: FAIL"},

    {"andone", 0, 0, 0, 11716, "1: AN Done 0: AN not done"},

};
reg_decoder_t lsmcpcs0_sts_flds = {2, lsmcpcs0_sts_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_fint0_fld_list[] = {
    {"ovrdint", 0, 0, 0, 11717, "AN Done Interrupt override"},

};
reg_decoder_t lsmcpcs0_fint0_flds = {1, lsmcpcs0_fint0_fld_list, 16};

reg_decoder_fld_t lsmcpcs0_spare1_fld_list[] = {
    {"spare1", 7, 0, 0, 11718, "Spare1"},

};
reg_decoder_t lsmcpcs0_spare1_flds = {1, lsmcpcs0_spare1_fld_list, 16};

reg_decoder_fld_t lsmcpcs1_ctrl_fld_list[] = {
    {"anrestart", 1, 1, 0, 11719, "1: restart Auto Negotiation"},

    {"anenable",
     0,
     0,
     0,
     11720,
     "1: Enable Auto Negotiation 0: Disable Auto Negotiation"},

};
reg_decoder_t lsmcpcs1_ctrl_flds = {2, lsmcpcs1_ctrl_fld_list, 16};

reg_decoder_fld_t lsmcpcs1_mradvability_fld_list[] = {
    {"basepg15",
     15,
     15,
     0,
     11721,
     "A register that contains the advertised ability base page of the local "
     "device to be conveyed to tx_config_reg<D15> for transmission to the link "
     "partner."},

    {"basepg",
     13,
     0,
     0,
     11722,
     "A 14-bit register that contains the advertised ability base page of the "
     "local device to be conveyed to tx_config_reg<D13:D0> for transmission to "
     "the link partner."},

};
reg_decoder_t lsmcpcs1_mradvability_flds = {
    2, lsmcpcs1_mradvability_fld_list, 16};

reg_decoder_fld_t lsmcpcs1_rxconfig_fld_list[] = {
    {"rxconfig", 15, 0, 0, 11723, "RX AN config"},

};
reg_decoder_t lsmcpcs1_rxconfig_flds = {1, lsmcpcs1_rxconfig_fld_list, 16};

reg_decoder_fld_t lsmcpcs1_lnktimer_fld_list[] = {
    {"lnktimer", 15, 0, 0, 11724, "default set to 10 ms for 125 MHz gb clk"},

};
reg_decoder_t lsmcpcs1_lnktimer_flds = {1, lsmcpcs1_lnktimer_fld_list, 16};

reg_decoder_fld_t lsmcpcs1_lnktimer2_fld_list[] = {
    {"lnktimer", 7, 0, 0, 11725, "default set to 10 ms for 125 MHz gb clk"},

};
reg_decoder_t lsmcpcs1_lnktimer2_flds = {1, lsmcpcs1_lnktimer2_fld_list, 16};

reg_decoder_fld_t lsmcpcs1_disperrcnt_fld_list[] = {
    {"disperrcnt",
     15,
     0,
     0,
     11726,
     "Disparity error count for channel N. Clears on read."},

};
reg_decoder_t lsmcpcs1_disperrcnt_flds = {1, lsmcpcs1_disperrcnt_fld_list, 16};

reg_decoder_fld_t lsmcpcs1_invcodecnt_fld_list[] = {
    {"invcodecnt",
     15,
     0,
     0,
     11727,
     "Invalid code count for channel N. Clears on read."},

};
reg_decoder_t lsmcpcs1_invcodecnt_flds = {1, lsmcpcs1_invcodecnt_fld_list, 16};

reg_decoder_fld_t lsmcpcs1_sts_fld_list[] = {
    {"syncstat", 1, 1, 0, 11728, "1: OK 0: FAIL"},

    {"andone", 0, 0, 0, 11729, "1: AN Done 0: AN not done"},

};
reg_decoder_t lsmcpcs1_sts_flds = {2, lsmcpcs1_sts_fld_list, 16};

reg_decoder_fld_t lsmcpcs1_fint0_fld_list[] = {
    {"ovrdint", 0, 0, 0, 11730, "AN Done Interrupt override"},

};
reg_decoder_t lsmcpcs1_fint0_flds = {1, lsmcpcs1_fint0_fld_list, 16};

reg_decoder_fld_t lsmcpcs2_ctrl_fld_list[] = {
    {"anrestart", 1, 1, 0, 11731, "1: restart Auto Negotiation"},

    {"anenable",
     0,
     0,
     0,
     11732,
     "1: Enable Auto Negotiation 0: Disable Auto Negotiation"},

};
reg_decoder_t lsmcpcs2_ctrl_flds = {2, lsmcpcs2_ctrl_fld_list, 16};

reg_decoder_fld_t lsmcpcs2_mradvability_fld_list[] = {
    {"basepg15",
     15,
     15,
     0,
     11733,
     "A register that contains the advertised ability base page of the local "
     "device to be conveyed to tx_config_reg<D15> for transmission to the link "
     "partner."},

    {"basepg",
     13,
     0,
     0,
     11734,
     "A 14-bit register that contains the advertised ability base page of the "
     "local device to be conveyed to tx_config_reg<D13:D0> for transmission to "
     "the link partner."},

};
reg_decoder_t lsmcpcs2_mradvability_flds = {
    2, lsmcpcs2_mradvability_fld_list, 16};

reg_decoder_fld_t lsmcpcs2_rxconfig_fld_list[] = {
    {"rxconfig", 15, 0, 0, 11735, "RX AN config"},

};
reg_decoder_t lsmcpcs2_rxconfig_flds = {1, lsmcpcs2_rxconfig_fld_list, 16};

reg_decoder_fld_t lsmcpcs2_lnktimer_fld_list[] = {
    {"lnktimer", 15, 0, 0, 11736, "default set to 10 ms for 125 MHz gb clk"},

};
reg_decoder_t lsmcpcs2_lnktimer_flds = {1, lsmcpcs2_lnktimer_fld_list, 16};

reg_decoder_fld_t lsmcpcs2_lnktimer2_fld_list[] = {
    {"lnktimer", 7, 0, 0, 11737, "default set to 10 ms for 125 MHz gb clk"},

};
reg_decoder_t lsmcpcs2_lnktimer2_flds = {1, lsmcpcs2_lnktimer2_fld_list, 16};

reg_decoder_fld_t lsmcpcs2_disperrcnt_fld_list[] = {
    {"disperrcnt",
     15,
     0,
     0,
     11738,
     "Disparity error count for channel N. Clears on read."},

};
reg_decoder_t lsmcpcs2_disperrcnt_flds = {1, lsmcpcs2_disperrcnt_fld_list, 16};

reg_decoder_fld_t lsmcpcs2_invcodecnt_fld_list[] = {
    {"invcodecnt",
     15,
     0,
     0,
     11739,
     "Invalid code count for channel N. Clears on read."},

};
reg_decoder_t lsmcpcs2_invcodecnt_flds = {1, lsmcpcs2_invcodecnt_fld_list, 16};

reg_decoder_fld_t lsmcpcs2_sts_fld_list[] = {
    {"syncstat", 1, 1, 0, 11740, "1: OK 0: FAIL"},

    {"andone", 0, 0, 0, 11741, "1: AN Done 0: AN not done"},

};
reg_decoder_t lsmcpcs2_sts_flds = {2, lsmcpcs2_sts_fld_list, 16};

reg_decoder_fld_t lsmcpcs2_fint0_fld_list[] = {
    {"ovrdint", 0, 0, 0, 11742, "AN Done Interrupt override"},

};
reg_decoder_t lsmcpcs2_fint0_flds = {1, lsmcpcs2_fint0_fld_list, 16};

reg_decoder_fld_t lsmcpcs3_ctrl_fld_list[] = {
    {"anrestart", 1, 1, 0, 11743, "1: restart Auto Negotiation"},

    {"anenable",
     0,
     0,
     0,
     11744,
     "1: Enable Auto Negotiation 0: Disable Auto Negotiation"},

};
reg_decoder_t lsmcpcs3_ctrl_flds = {2, lsmcpcs3_ctrl_fld_list, 16};

reg_decoder_fld_t lsmcpcs3_mradvability_fld_list[] = {
    {"basepg15",
     15,
     15,
     0,
     11745,
     "A register that contains the advertised ability base page of the local "
     "device to be conveyed to tx_config_reg<D15> for transmission to the link "
     "partner."},

    {"basepg",
     13,
     0,
     0,
     11746,
     "A 14-bit register that contains the advertised ability base page of the "
     "local device to be conveyed to tx_config_reg<D13:D0> for transmission to "
     "the link partner."},

};
reg_decoder_t lsmcpcs3_mradvability_flds = {
    2, lsmcpcs3_mradvability_fld_list, 16};

reg_decoder_fld_t lsmcpcs3_rxconfig_fld_list[] = {
    {"rxconfig", 15, 0, 0, 11747, "RX AN config"},

};
reg_decoder_t lsmcpcs3_rxconfig_flds = {1, lsmcpcs3_rxconfig_fld_list, 16};

reg_decoder_fld_t lsmcpcs3_lnktimer_fld_list[] = {
    {"lnktimer", 15, 0, 0, 11748, "default set to 10 ms for 125 MHz gb clk"},

};
reg_decoder_t lsmcpcs3_lnktimer_flds = {1, lsmcpcs3_lnktimer_fld_list, 16};

reg_decoder_fld_t lsmcpcs3_lnktimer2_fld_list[] = {
    {"lnktimer", 7, 0, 0, 11749, "default set to 10 ms for 125 MHz gb clk"},

};
reg_decoder_t lsmcpcs3_lnktimer2_flds = {1, lsmcpcs3_lnktimer2_fld_list, 16};

reg_decoder_fld_t lsmcpcs3_disperrcnt_fld_list[] = {
    {"disperrcnt",
     15,
     0,
     0,
     11750,
     "Disparity error count for channel N. Clears on read."},

};
reg_decoder_t lsmcpcs3_disperrcnt_flds = {1, lsmcpcs3_disperrcnt_fld_list, 16};

reg_decoder_fld_t lsmcpcs3_invcodecnt_fld_list[] = {
    {"invcodecnt",
     15,
     0,
     0,
     11751,
     "Invalid code count for channel N. Clears on read."},

};
reg_decoder_t lsmcpcs3_invcodecnt_flds = {1, lsmcpcs3_invcodecnt_fld_list, 16};

reg_decoder_fld_t lsmcpcs3_sts_fld_list[] = {
    {"syncstat", 1, 1, 0, 11752, "1: OK 0: FAIL"},

    {"andone", 0, 0, 0, 11753, "1: AN Done 0: AN not done"},

};
reg_decoder_t lsmcpcs3_sts_flds = {2, lsmcpcs3_sts_fld_list, 16};

reg_decoder_fld_t lsmcpcs3_fint0_fld_list[] = {
    {"ovrdint", 0, 0, 0, 11754, "AN Done Interrupt override"},

};
reg_decoder_t lsmcpcs3_fint0_flds = {1, lsmcpcs3_fint0_fld_list, 16};

reg_decoder_fld_t bpan0_ctrl1_fld_list[] = {
    {"ansetmode", 0, 0, 0, 11755, "Set operation mode 0: Idle 1: Normal"},

    {"andisablepn",
     1,
     1,
     0,
     11756,
     "Disable PN sequence of bit 49. 0: enable 1: disable"},

    {"anarbenable",
     2,
     2,
     0,
     11757,
     "Enable the AN arbitration function. 0: disable 1: enable"},

    {"antransmiten",
     8,
     8,
     0,
     11758,
     "Control the enable of the AN transmitter when the AN_arb_enable is "
     "disabled 0: disable 1: enable"},

    {"anrestart", 9, 9, 0, 11759, "Restart autonegotiation"},

    {"anreceiveen",
     10,
     10,
     0,
     11760,
     "Control the enable of the AN receiver when the AN_arb_enable is disabled "
     "0: disable 1: enable"},

    {"anrxgbresync",
     11,
     11,
     0,
     11761,
     "Receive gear box resync, manually force re-initialization ofe the edge "
     "detector for the Receive Gear Box 0: disable 1: enable"},

    {"anenable",
     12,
     12,
     0,
     11762,
     "Enable the AN function.  Mainly used for clock gating of the entire "
     "block. 0: disable 1: enable AN"},

    {"anbasepage",
     13,
     13,
     0,
     11763,
     "Base page transfer enable for software control mode.  When this bit is "
     "set to base page, the base page register will be used for transmission "
     "and reception.  Otherwise, use the next page registers. 0: base page "
     "transfer 1: next page transfer"},

    {"anrxgbresyncen",
     14,
     14,
     0,
     11764,
     "Receive gear box resync, allow hardware controlled re-initialization of "
     "the edge detector for the Receive Gear Box 0: disable 1: enable"},

    {"ansoftrstn", 15, 15, 0, 11765, "Soft reset 0: enable 1: disable"},

};
reg_decoder_t bpan0_ctrl1_flds = {11, bpan0_ctrl1_fld_list, 16};

reg_decoder_fld_t bpan0_status1_fld_list[] = {
    {"lpanenable",
     0,
     0,
     0,
     11766,
     "Link Partner autonegotiationable 0: Negotiation has not begun 1: "
     "Negotiation has begun"},

    {"anability",
     3,
     3,
     0,
     11767,
     "Used to signal that this design has BPAN capabilities"},

    {"ancomplete", 5, 5, 0, 11768, "Autonegotiation complete"},

    {"pagerx", 6, 6, 0, 11769, "Page received"},

    {"nploaded", 10, 10, 0, 11770, "Next page loaded"},

};
reg_decoder_t bpan0_status1_flds = {5, bpan0_status1_fld_list, 16};

reg_decoder_fld_t bpan0_tnonce_fld_list[] = {
    {"antnonceforceval",
     4,
     0,
     0,
     11771,
     "Value forced into the transmitted Nonce Field."},

    {"antnonceforceen",
     5,
     5,
     0,
     11772,
     "Enable the forcing of transmitter Nonce field for debugging purposes. "
     "0:disable 1:enable"},

};
reg_decoder_t bpan0_tnonce_flds = {2, bpan0_tnonce_fld_list, 16};

reg_decoder_fld_t bpan0_enonce_fld_list[] = {
    {"anforceenonceval",
     4,
     0,
     0,
     11773,
     "Section 73.6.2. The forced value for the echo."},

    {"anforceenonceen",
     5,
     5,
     0,
     11774,
     "Section 73.6.2, enable the forcing of echoed nonce value instead of "
     "using echoed value. 0: disable 1: enable"},

};
reg_decoder_t bpan0_enonce_flds = {2, bpan0_enonce_fld_list, 16};

reg_decoder_fld_t bpan0_abrctrl1_fld_list[] = {
    {"breaklinktimer",
     15,
     0,
     0,
     11775,
     "Timer for the amount of time to wait in order to assure that the link "
     "partner enters a Link Fail state.  The timer expires  60ms to 75ms after "
     "being started.  Section 73.10.2"},

};
reg_decoder_t bpan0_abrctrl1_flds = {1, bpan0_abrctrl1_fld_list, 16};

reg_decoder_fld_t bpan0_abrctrl2_fld_list[] = {
    {"breaklinktimer",
     15,
     0,
     0,
     11776,
     "Timer for the amount of time to wait in order to assure that the link "
     "partner enters a Link Fail state.  The timer expires  60ms to 75ms after "
     "being started.  Section 73.10.2"},

};
reg_decoder_t bpan0_abrctrl2_flds = {1, bpan0_abrctrl2_fld_list, 16};

reg_decoder_fld_t bpan0_pagetestmaxtimer_fld_list[] = {
    {"pagetestmaxtimer",
     15,
     0,
     0,
     11777,
     "Timer for the maximum time between detection of Manchester violation "
     "delimiters.  This timer is used in conjunction with the "
     "page_test_min_timer to detect whether the link partner is transmitting "
     "DME pages.  The page_test_max_timer expires 350ns to 375ns after being "
     "started or restarted.  Section 73.10.2"},

};
reg_decoder_t bpan0_pagetestmaxtimer_flds = {
    1, bpan0_pagetestmaxtimer_fld_list, 16};

reg_decoder_fld_t bpan0_pagetestmintimer_fld_list[] = {
    {"pagetestmintimer",
     15,
     0,
     0,
     11778,
     "Timer for the minimum time between detection of Manchester violation "
     "delimiters.  This timer is used in conjunction with the "
     "page_test_max_timer to detect whether the link partner is transmitting "
     "DME pages.  The page_test_min_timer expires 305ns to 330ns after being "
     "started or restarted.  Section 73.10.2"},

};
reg_decoder_t bpan0_pagetestmintimer_flds = {
    1, bpan0_pagetestmintimer_fld_list, 16};

reg_decoder_fld_t bpan0_txbasepagelo_fld_list[] = {
    {"txselector", 4, 0, 0, 11779, "Selector field, Section 73.6.1"},

    {"txechoednonce", 9, 5, 0, 11780, "Echoed Nonce Field, Section 73.6.2"},

    {"txpausecap", 11, 10, 0, 11781, "Section 73.6.6."},

    {"txremotefault", 13, 13, 0, 11782, "Section 73.6.7"},

    {"txack", 14, 14, 0, 11783, "Section 73.6.8"},

    {"txnp", 15, 15, 0, 11784, "Section 73.6.9"},

};
reg_decoder_t bpan0_txbasepagelo_flds = {6, bpan0_txbasepagelo_fld_list, 16};

reg_decoder_fld_t bpan0_txbasepagemid_fld_list[] = {
    {"txnonce",
     4,
     0,
     0,
     11785,
     "Transmitter Nonce field, Section 73.6.3. This is a randomly generated "
     "value. "},

    {"txtechability",
     15,
     5,
     0,
     11786,
     "Section 73.6.4 (802.3bj). One Hot encoding for the supporting technology "
     "based on 802.3bj.  Bit0 : 1000BASE-KX Bit1 : 10GBASE-KX4 Bit2 : "
     "10GBASE-KR Bit3 : 40GBASE-KR4 Bit4 : 40GBASE-CR4 Bit5 : 100GBASE-CR10 "
     "Bit6 : 100GBASE-KP4 Bit7 : 100GBASE-KR4 Bit8 : 100GBASE-CR4  Bit 9~15 "
     "are reserved in the 802.3bj spec.  Note: A compile-time parameterized "
     "mask can be used to control support of auto-negotiation when the "
     "auto-negotiation block is instantiated."},

};
reg_decoder_t bpan0_txbasepagemid_flds = {2, bpan0_txbasepagemid_fld_list, 16};

reg_decoder_fld_t bpan0_txbasepagehi_fld_list[] = {
    {"txtechability",
     13,
     0,
     0,
     11787,
     "Corresponds to tech ability [23:16]. Bit 16~23 are reserved in the "
     "802.3bj spec."},

    {"txfecability",
     15,
     14,
     0,
     11788,
     "Section 73.6.5. FEC capability. Bit[1] is to request FEC. 0: No FEC "
     "request. 1: Request FEC on link. Bit[0] is FEC support.  0: No FEC "
     "supported. 1: FEC supported."},

};
reg_decoder_t bpan0_txbasepagehi_flds = {2, bpan0_txbasepagehi_fld_list, 16};

reg_decoder_fld_t bpan0_rxbasepagelo_fld_list[] = {
    {"rxselector", 4, 0, 0, 11789, "Selector field, Section 73.6.1"},

    {"rxechoednonce", 9, 5, 0, 11790, "Echoed Nonce Field, Section 73.6.2"},

    {"rxpausecap", 11, 10, 0, 11791, "Section 73.6.6."},

    {"rxremotefault", 13, 13, 0, 11792, "Section 73.6.7"},

    {"rxack", 14, 14, 0, 11793, "Section 73.6.8"},

    {"rxnp", 15, 15, 0, 11794, "Section 73.6.9"},

};
reg_decoder_t bpan0_rxbasepagelo_flds = {6, bpan0_rxbasepagelo_fld_list, 16};

reg_decoder_fld_t bpan0_rxbasepagemid_fld_list[] = {
    {"rxnonce",
     4,
     0,
     0,
     11795,
     "Transmitter Nonce field, Section 73.6.3. This is a randomly generated "
     "value. "},

    {"rxtechability",
     15,
     5,
     0,
     11796,
     "Section 73.6.4 (802.3bj). One Hot encoding for the supporting technology "
     "based on 802.3bj.  Bit0 : 1000BASE-KX Bit1 : 10GBASE-KX4 Bit2 : "
     "10GBASE-KR Bit3 : 40GBASE-KR4 Bit4 : 40GBASE-CR4 Bit5 : 100GBASE-CR10 "
     "Bit6 : 100GBASE-KP4 Bit7 : 100GBASE-KR4 Bit8 : 100GBASE-CR4  Bit 9~15 "
     "are reserved in the 802.3bj spec.  Note: A compile-time parameterized "
     "mask can be used to control support of auto-negotiation when the "
     "auto-negotiation block is instantiated."},

};
reg_decoder_t bpan0_rxbasepagemid_flds = {2, bpan0_rxbasepagemid_fld_list, 16};

reg_decoder_fld_t bpan0_rxbasepagehi_fld_list[] = {
    {"rxtechability",
     13,
     0,
     0,
     11797,
     "Corresponds to tech ability [23:16]. Bit 16~23 are reserved in the "
     "802.3bj spec."},

    {"rxfecability",
     15,
     14,
     0,
     11798,
     "Section 73.6.5. FEC capability. Bit[1] is to request FEC. 0: No FEC "
     "request. 1: Request FEC on link. Bit[0] is FEC support.  0: No FEC "
     "supported. 1: FEC supported."},

};
reg_decoder_t bpan0_rxbasepagehi_flds = {2, bpan0_rxbasepagehi_fld_list, 16};

reg_decoder_fld_t bpan0_txnextpagelo_fld_list[] = {
    {"txlowercodefield",
     10,
     0,
     0,
     11799,
     "Message code field or Unformatted code field (section 73.7.7.1)"},

    {"txtoggleenable",
     11,
     11,
     0,
     11800,
     "Control updates to tx_toggle and tx_complete 0x0: Disallow updates 0x1: "
     "Allow updates"},

    {"txacknowledge2",
     12,
     12,
     0,
     11801,
     "Acknowledge 2.When the AN_arb_enable is set, hardware will generate the "
     "acknowledge_2 message."},

    {"txmp",
     13,
     13,
     0,
     11802,
     "Message page 0x0: Next page is an unformatted code field 0x1: Next page "
     "is a message code field"},

    {"txacknowledge",
     14,
     14,
     0,
     11803,
     "Acknowledge.When AN_arb_enable is set, hardware will generate the "
     "acknowledge message."},

    {"txnextpage",
     15,
     15,
     0,
     11804,
     "Next page indication 0x0: Next page is not available 0x1: Next page is "
     "available"},

};
reg_decoder_t bpan0_txnextpagelo_flds = {6, bpan0_txnextpagelo_fld_list, 16};

reg_decoder_fld_t bpan0_txnextpagemid_fld_list[] = {
    {"txunformattedcode",
     15,
     0,
     0,
     11805,
     "Unformatted code field lower word.  This field corresponds  to bit 16: "
     "31 in the DME next page"},

};
reg_decoder_t bpan0_txnextpagemid_flds = {1, bpan0_txnextpagemid_fld_list, 16};

reg_decoder_fld_t bpan0_txnextpagehi_fld_list[] = {
    {"txunformattedcode",
     15,
     0,
     0,
     11806,
     "Unformatted code field upper word.  This field corresponds to bit 32: 47 "
     "in the DME next page"},

};
reg_decoder_t bpan0_txnextpagehi_flds = {1, bpan0_txnextpagehi_fld_list, 16};

reg_decoder_fld_t bpan0_rxnextpagelo_fld_list[] = {
    {"rxlowercodefield",
     10,
     0,
     0,
     11807,
     "Message code field or Unformatted code field (section 73.7.7.1)"},

    {"rxtoggleenable",
     11,
     11,
     0,
     11808,
     "Control updates to rx_toggle and rx_complete 0x0: Disallow updates 0x1: "
     "Allow updates"},

    {"rxacknowledge2",
     12,
     12,
     0,
     11809,
     "Acknowledge 2.When the AN_arb_enable is set, hardware will generate the "
     "acknowledge_2 message."},

    {"rxmp",
     13,
     13,
     0,
     11810,
     "Message page 0x0: Next page is an unformatted code field 0x1: Next page "
     "is a message code field"},

    {"rxacknowledge",
     14,
     14,
     0,
     11811,
     "Acknowledge.When AN_arb_enable is set, hardware will generate the "
     "acknowledge message."},

    {"rxnextpage",
     15,
     15,
     0,
     11812,
     "Next page indication 0x0: Next page is not available 0x1: Next page is "
     "available"},

};
reg_decoder_t bpan0_rxnextpagelo_flds = {6, bpan0_rxnextpagelo_fld_list, 16};

reg_decoder_fld_t bpan0_rxnextpagemid_fld_list[] = {
    {"rxunformattedcode",
     15,
     0,
     0,
     11813,
     "Unformatted code field lower word.  This field corresponds  to bit 16: "
     "31 in the DME next page"},

};
reg_decoder_t bpan0_rxnextpagemid_flds = {1, bpan0_rxnextpagemid_fld_list, 16};

reg_decoder_fld_t bpan0_rxnextpagehi_fld_list[] = {
    {"rxunformattedcode",
     15,
     0,
     0,
     11814,
     "Unformatted code field upper word.  This field corresponds to bit 32: 47 "
     "in the DME next page"},

};
reg_decoder_t bpan0_rxnextpagehi_flds = {1, bpan0_rxnextpagehi_fld_list, 16};

reg_decoder_fld_t bpan0_fint0_fld_list[] = {
    {"ovrdint", 5, 0, 0, 11815, "BPAN TX base page Interrupt override"},

};
reg_decoder_t bpan0_fint0_flds = {1, bpan0_fint0_fld_list, 16};

reg_decoder_fld_t bpan0_intstatus_fld_list[] = {
    {"txbp", 0, 0, 0, 11816, "Transmitted Base Page status"},

    {"txnp", 1, 1, 0, 11817, "Transmitted Next Page status"},

    {"txack", 2, 2, 0, 11818, "Transmitted Page Acknowledge status"},

    {"rxbp", 3, 3, 0, 11819, "Received Base Page status"},

    {"rxnp", 4, 4, 0, 11820, "Received Next Page status"},

    {"ancomplete", 5, 5, 0, 11821, "Autonegotiation complete status"},

};
reg_decoder_t bpan0_intstatus_flds = {6, bpan0_intstatus_fld_list, 16};

reg_decoder_fld_t bpan1_ctrl1_fld_list[] = {
    {"ansetmode", 0, 0, 0, 11822, "Set operation mode 0: Idle 1: Normal"},

    {"andisablepn",
     1,
     1,
     0,
     11823,
     "Disable PN sequence of bit 49. 0: enable 1: disable"},

    {"anarbenable",
     2,
     2,
     0,
     11824,
     "Enable the AN arbitration function. 0: disable 1: enable"},

    {"antransmiten",
     8,
     8,
     0,
     11825,
     "Control the enable of the AN transmitter when the AN_arb_enable is "
     "disabled 0: disable 1: enable"},

    {"anrestart", 9, 9, 0, 11826, "Restart autonegotiation"},

    {"anreceiveen",
     10,
     10,
     0,
     11827,
     "Control the enable of the AN receiver when the AN_arb_enable is disabled "
     "0: disable 1: enable"},

    {"anrxgbresync",
     11,
     11,
     0,
     11828,
     "Receive gear box resync, manually force re-initialization ofe the edge "
     "detector for the Receive Gear Box 0: disable 1: enable"},

    {"anenable",
     12,
     12,
     0,
     11829,
     "Enable the AN function.  Mainly used for clock gating of the entire "
     "block. 0: disable 1: enable AN"},

    {"anbasepage",
     13,
     13,
     0,
     11830,
     "Base page transfer enable for software control mode.  When this bit is "
     "set to base page, the base page register will be used for transmission "
     "and reception.  Otherwise, use the next page registers. 0: base page "
     "transfer 1: next page transfer"},

    {"anrxgbresyncen",
     14,
     14,
     0,
     11831,
     "Receive gear box resync, allow hardware controlled re-initialization of "
     "the edge detector for the Receive Gear Box 0: disable 1: enable"},

    {"ansoftrstn", 15, 15, 0, 11832, "Soft reset 0: enable 1: disable"},

};
reg_decoder_t bpan1_ctrl1_flds = {11, bpan1_ctrl1_fld_list, 16};

reg_decoder_fld_t bpan1_status1_fld_list[] = {
    {"lpanenable",
     0,
     0,
     0,
     11833,
     "Link Partner autonegotiationable 0: Negotiation has not begun 1: "
     "Negotiation has begun"},

    {"anability",
     3,
     3,
     0,
     11834,
     "Used to signal that this design has BPAN capabilities"},

    {"ancomplete", 5, 5, 0, 11835, "Autonegotiation complete"},

    {"pagerx", 6, 6, 0, 11836, "Page received"},

    {"nploaded", 10, 10, 0, 11837, "Next page loaded"},

};
reg_decoder_t bpan1_status1_flds = {5, bpan1_status1_fld_list, 16};

reg_decoder_fld_t bpan1_tnonce_fld_list[] = {
    {"antnonceforceval",
     4,
     0,
     0,
     11838,
     "Value forced into the transmitted Nonce Field."},

    {"antnonceforceen",
     5,
     5,
     0,
     11839,
     "Enable the forcing of transmitter Nonce field for debugging purposes. "
     "0:disable 1:enable"},

};
reg_decoder_t bpan1_tnonce_flds = {2, bpan1_tnonce_fld_list, 16};

reg_decoder_fld_t bpan1_enonce_fld_list[] = {
    {"anforceenonceval",
     4,
     0,
     0,
     11840,
     "Section 73.6.2. The forced value for the echo."},

    {"anforceenonceen",
     5,
     5,
     0,
     11841,
     "Section 73.6.2, enable the forcing of echoed nonce value instead of "
     "using echoed value. 0: disable 1: enable"},

};
reg_decoder_t bpan1_enonce_flds = {2, bpan1_enonce_fld_list, 16};

reg_decoder_fld_t bpan1_abrctrl1_fld_list[] = {
    {"breaklinktimer",
     15,
     0,
     0,
     11842,
     "Timer for the amount of time to wait in order to assure that the link "
     "partner enters a Link Fail state.  The timer expires  60ms to 75ms after "
     "being started.  Section 73.10.2"},

};
reg_decoder_t bpan1_abrctrl1_flds = {1, bpan1_abrctrl1_fld_list, 16};

reg_decoder_fld_t bpan1_abrctrl2_fld_list[] = {
    {"breaklinktimer",
     15,
     0,
     0,
     11843,
     "Timer for the amount of time to wait in order to assure that the link "
     "partner enters a Link Fail state.  The timer expires  60ms to 75ms after "
     "being started.  Section 73.10.2"},

};
reg_decoder_t bpan1_abrctrl2_flds = {1, bpan1_abrctrl2_fld_list, 16};

reg_decoder_fld_t bpan1_pagetestmaxtimer_fld_list[] = {
    {"pagetestmaxtimer",
     15,
     0,
     0,
     11844,
     "Timer for the maximum time between detection of Manchester violation "
     "delimiters.  This timer is used in conjunction with the "
     "page_test_min_timer to detect whether the link partner is transmitting "
     "DME pages.  The page_test_max_timer expires 350ns to 375ns after being "
     "started or restarted.  Section 73.10.2"},

};
reg_decoder_t bpan1_pagetestmaxtimer_flds = {
    1, bpan1_pagetestmaxtimer_fld_list, 16};

reg_decoder_fld_t bpan1_pagetestmintimer_fld_list[] = {
    {"pagetestmintimer",
     15,
     0,
     0,
     11845,
     "Timer for the minimum time between detection of Manchester violation "
     "delimiters.  This timer is used in conjunction with the "
     "page_test_max_timer to detect whether the link partner is transmitting "
     "DME pages.  The page_test_min_timer expires 305ns to 330ns after being "
     "started or restarted.  Section 73.10.2"},

};
reg_decoder_t bpan1_pagetestmintimer_flds = {
    1, bpan1_pagetestmintimer_fld_list, 16};

reg_decoder_fld_t bpan1_txbasepagelo_fld_list[] = {
    {"txselector", 4, 0, 0, 11846, "Selector field, Section 73.6.1"},

    {"txechoednonce", 9, 5, 0, 11847, "Echoed Nonce Field, Section 73.6.2"},

    {"txpausecap", 11, 10, 0, 11848, "Section 73.6.6."},

    {"txremotefault", 13, 13, 0, 11849, "Section 73.6.7"},

    {"txack", 14, 14, 0, 11850, "Section 73.6.8"},

    {"txnp", 15, 15, 0, 11851, "Section 73.6.9"},

};
reg_decoder_t bpan1_txbasepagelo_flds = {6, bpan1_txbasepagelo_fld_list, 16};

reg_decoder_fld_t bpan1_txbasepagemid_fld_list[] = {
    {"txnonce",
     4,
     0,
     0,
     11852,
     "Transmitter Nonce field, Section 73.6.3. This is a randomly generated "
     "value. "},

    {"txtechability",
     15,
     5,
     0,
     11853,
     "Section 73.6.4 (802.3bj). One Hot encoding for the supporting technology "
     "based on 802.3bj.  Bit0 : 1000BASE-KX Bit1 : 10GBASE-KX4 Bit2 : "
     "10GBASE-KR Bit3 : 40GBASE-KR4 Bit4 : 40GBASE-CR4 Bit5 : 100GBASE-CR10 "
     "Bit6 : 100GBASE-KP4 Bit7 : 100GBASE-KR4 Bit8 : 100GBASE-CR4  Bit 9~15 "
     "are reserved in the 802.3bj spec.  Note: A compile-time parameterized "
     "mask can be used to control support of auto-negotiation when the "
     "auto-negotiation block is instantiated."},

};
reg_decoder_t bpan1_txbasepagemid_flds = {2, bpan1_txbasepagemid_fld_list, 16};

reg_decoder_fld_t bpan1_txbasepagehi_fld_list[] = {
    {"txtechability",
     13,
     0,
     0,
     11854,
     "Corresponds to tech ability [23:16]. Bit 16~23 are reserved in the "
     "802.3bj spec."},

    {"txfecability",
     15,
     14,
     0,
     11855,
     "Section 73.6.5. FEC capability. Bit[1] is to request FEC. 0: No FEC "
     "request. 1: Request FEC on link. Bit[0] is FEC support.  0: No FEC "
     "supported. 1: FEC supported."},

};
reg_decoder_t bpan1_txbasepagehi_flds = {2, bpan1_txbasepagehi_fld_list, 16};

reg_decoder_fld_t bpan1_rxbasepagelo_fld_list[] = {
    {"rxselector", 4, 0, 0, 11856, "Selector field, Section 73.6.1"},

    {"rxechoednonce", 9, 5, 0, 11857, "Echoed Nonce Field, Section 73.6.2"},

    {"rxpausecap", 11, 10, 0, 11858, "Section 73.6.6."},

    {"rxremotefault", 13, 13, 0, 11859, "Section 73.6.7"},

    {"rxack", 14, 14, 0, 11860, "Section 73.6.8"},

    {"rxnp", 15, 15, 0, 11861, "Section 73.6.9"},

};
reg_decoder_t bpan1_rxbasepagelo_flds = {6, bpan1_rxbasepagelo_fld_list, 16};

reg_decoder_fld_t bpan1_rxbasepagemid_fld_list[] = {
    {"rxnonce",
     4,
     0,
     0,
     11862,
     "Transmitter Nonce field, Section 73.6.3. This is a randomly generated "
     "value. "},

    {"rxtechability",
     15,
     5,
     0,
     11863,
     "Section 73.6.4 (802.3bj). One Hot encoding for the supporting technology "
     "based on 802.3bj.  Bit0 : 1000BASE-KX Bit1 : 10GBASE-KX4 Bit2 : "
     "10GBASE-KR Bit3 : 40GBASE-KR4 Bit4 : 40GBASE-CR4 Bit5 : 100GBASE-CR10 "
     "Bit6 : 100GBASE-KP4 Bit7 : 100GBASE-KR4 Bit8 : 100GBASE-CR4  Bit 9~15 "
     "are reserved in the 802.3bj spec.  Note: A compile-time parameterized "
     "mask can be used to control support of auto-negotiation when the "
     "auto-negotiation block is instantiated."},

};
reg_decoder_t bpan1_rxbasepagemid_flds = {2, bpan1_rxbasepagemid_fld_list, 16};

reg_decoder_fld_t bpan1_rxbasepagehi_fld_list[] = {
    {"rxtechability",
     13,
     0,
     0,
     11864,
     "Corresponds to tech ability [23:16]. Bit 16~23 are reserved in the "
     "802.3bj spec."},

    {"rxfecability",
     15,
     14,
     0,
     11865,
     "Section 73.6.5. FEC capability. Bit[1] is to request FEC. 0: No FEC "
     "request. 1: Request FEC on link. Bit[0] is FEC support.  0: No FEC "
     "supported. 1: FEC supported."},

};
reg_decoder_t bpan1_rxbasepagehi_flds = {2, bpan1_rxbasepagehi_fld_list, 16};

reg_decoder_fld_t bpan1_txnextpagelo_fld_list[] = {
    {"txlowercodefield",
     10,
     0,
     0,
     11866,
     "Message code field or Unformatted code field (section 73.7.7.1)"},

    {"txtoggleenable",
     11,
     11,
     0,
     11867,
     "Control updates to tx_toggle and tx_complete 0x0: Disallow updates 0x1: "
     "Allow updates"},

    {"txacknowledge2",
     12,
     12,
     0,
     11868,
     "Acknowledge 2.When the AN_arb_enable is set, hardware will generate the "
     "acknowledge_2 message."},

    {"txmp",
     13,
     13,
     0,
     11869,
     "Message page 0x0: Next page is an unformatted code field 0x1: Next page "
     "is a message code field"},

    {"txacknowledge",
     14,
     14,
     0,
     11870,
     "Acknowledge.When AN_arb_enable is set, hardware will generate the "
     "acknowledge message."},

    {"txnextpage",
     15,
     15,
     0,
     11871,
     "Next page indication 0x0: Next page is not available 0x1: Next page is "
     "available"},

};
reg_decoder_t bpan1_txnextpagelo_flds = {6, bpan1_txnextpagelo_fld_list, 16};

reg_decoder_fld_t bpan1_txnextpagemid_fld_list[] = {
    {"txunformattedcode",
     15,
     0,
     0,
     11872,
     "Unformatted code field lower word.  This field corresponds  to bit 16: "
     "31 in the DME next page"},

};
reg_decoder_t bpan1_txnextpagemid_flds = {1, bpan1_txnextpagemid_fld_list, 16};

reg_decoder_fld_t bpan1_txnextpagehi_fld_list[] = {
    {"txunformattedcode",
     15,
     0,
     0,
     11873,
     "Unformatted code field upper word.  This field corresponds to bit 32: 47 "
     "in the DME next page"},

};
reg_decoder_t bpan1_txnextpagehi_flds = {1, bpan1_txnextpagehi_fld_list, 16};

reg_decoder_fld_t bpan1_rxnextpagelo_fld_list[] = {
    {"rxlowercodefield",
     10,
     0,
     0,
     11874,
     "Message code field or Unformatted code field (section 73.7.7.1)"},

    {"rxtoggleenable",
     11,
     11,
     0,
     11875,
     "Control updates to rx_toggle and rx_complete 0x0: Disallow updates 0x1: "
     "Allow updates"},

    {"rxacknowledge2",
     12,
     12,
     0,
     11876,
     "Acknowledge 2.When the AN_arb_enable is set, hardware will generate the "
     "acknowledge_2 message."},

    {"rxmp",
     13,
     13,
     0,
     11877,
     "Message page 0x0: Next page is an unformatted code field 0x1: Next page "
     "is a message code field"},

    {"rxacknowledge",
     14,
     14,
     0,
     11878,
     "Acknowledge.When AN_arb_enable is set, hardware will generate the "
     "acknowledge message."},

    {"rxnextpage",
     15,
     15,
     0,
     11879,
     "Next page indication 0x0: Next page is not available 0x1: Next page is "
     "available"},

};
reg_decoder_t bpan1_rxnextpagelo_flds = {6, bpan1_rxnextpagelo_fld_list, 16};

reg_decoder_fld_t bpan1_rxnextpagemid_fld_list[] = {
    {"rxunformattedcode",
     15,
     0,
     0,
     11880,
     "Unformatted code field lower word.  This field corresponds  to bit 16: "
     "31 in the DME next page"},

};
reg_decoder_t bpan1_rxnextpagemid_flds = {1, bpan1_rxnextpagemid_fld_list, 16};

reg_decoder_fld_t bpan1_rxnextpagehi_fld_list[] = {
    {"rxunformattedcode",
     15,
     0,
     0,
     11881,
     "Unformatted code field upper word.  This field corresponds to bit 32: 47 "
     "in the DME next page"},

};
reg_decoder_t bpan1_rxnextpagehi_flds = {1, bpan1_rxnextpagehi_fld_list, 16};

reg_decoder_fld_t bpan1_fint0_fld_list[] = {
    {"ovrdint", 5, 0, 0, 11882, "BPAN TX base page Interrupt override"},

};
reg_decoder_t bpan1_fint0_flds = {1, bpan1_fint0_fld_list, 16};

reg_decoder_fld_t bpan1_intstatus_fld_list[] = {
    {"txbp", 0, 0, 0, 11883, "Transmitted Base Page status"},

    {"txnp", 1, 1, 0, 11884, "Transmitted Next Page status"},

    {"txack", 2, 2, 0, 11885, "Transmitted Page Acknowledge status"},

    {"rxbp", 3, 3, 0, 11886, "Received Base Page status"},

    {"rxnp", 4, 4, 0, 11887, "Received Next Page status"},

    {"ancomplete", 5, 5, 0, 11888, "Autonegotiation complete status"},

};
reg_decoder_t bpan1_intstatus_flds = {6, bpan1_intstatus_fld_list, 16};

reg_decoder_fld_t bpan2_ctrl1_fld_list[] = {
    {"ansetmode", 0, 0, 0, 11889, "Set operation mode 0: Idle 1: Normal"},

    {"andisablepn",
     1,
     1,
     0,
     11890,
     "Disable PN sequence of bit 49. 0: enable 1: disable"},

    {"anarbenable",
     2,
     2,
     0,
     11891,
     "Enable the AN arbitration function. 0: disable 1: enable"},

    {"antransmiten",
     8,
     8,
     0,
     11892,
     "Control the enable of the AN transmitter when the AN_arb_enable is "
     "disabled 0: disable 1: enable"},

    {"anrestart", 9, 9, 0, 11893, "Restart autonegotiation"},

    {"anreceiveen",
     10,
     10,
     0,
     11894,
     "Control the enable of the AN receiver when the AN_arb_enable is disabled "
     "0: disable 1: enable"},

    {"anrxgbresync",
     11,
     11,
     0,
     11895,
     "Receive gear box resync, manually force re-initialization ofe the edge "
     "detector for the Receive Gear Box 0: disable 1: enable"},

    {"anenable",
     12,
     12,
     0,
     11896,
     "Enable the AN function.  Mainly used for clock gating of the entire "
     "block. 0: disable 1: enable AN"},

    {"anbasepage",
     13,
     13,
     0,
     11897,
     "Base page transfer enable for software control mode.  When this bit is "
     "set to base page, the base page register will be used for transmission "
     "and reception.  Otherwise, use the next page registers. 0: base page "
     "transfer 1: next page transfer"},

    {"anrxgbresyncen",
     14,
     14,
     0,
     11898,
     "Receive gear box resync, allow hardware controlled re-initialization of "
     "the edge detector for the Receive Gear Box 0: disable 1: enable"},

    {"ansoftrstn", 15, 15, 0, 11899, "Soft reset 0: enable 1: disable"},

};
reg_decoder_t bpan2_ctrl1_flds = {11, bpan2_ctrl1_fld_list, 16};

reg_decoder_fld_t bpan2_status1_fld_list[] = {
    {"lpanenable",
     0,
     0,
     0,
     11900,
     "Link Partner autonegotiationable 0: Negotiation has not begun 1: "
     "Negotiation has begun"},

    {"anability",
     3,
     3,
     0,
     11901,
     "Used to signal that this design has BPAN capabilities"},

    {"ancomplete", 5, 5, 0, 11902, "Autonegotiation complete"},

    {"pagerx", 6, 6, 0, 11903, "Page received"},

    {"nploaded", 10, 10, 0, 11904, "Next page loaded"},

};
reg_decoder_t bpan2_status1_flds = {5, bpan2_status1_fld_list, 16};

reg_decoder_fld_t bpan2_tnonce_fld_list[] = {
    {"antnonceforceval",
     4,
     0,
     0,
     11905,
     "Value forced into the transmitted Nonce Field."},

    {"antnonceforceen",
     5,
     5,
     0,
     11906,
     "Enable the forcing of transmitter Nonce field for debugging purposes. "
     "0:disable 1:enable"},

};
reg_decoder_t bpan2_tnonce_flds = {2, bpan2_tnonce_fld_list, 16};

reg_decoder_fld_t bpan2_enonce_fld_list[] = {
    {"anforceenonceval",
     4,
     0,
     0,
     11907,
     "Section 73.6.2. The forced value for the echo."},

    {"anforceenonceen",
     5,
     5,
     0,
     11908,
     "Section 73.6.2, enable the forcing of echoed nonce value instead of "
     "using echoed value. 0: disable 1: enable"},

};
reg_decoder_t bpan2_enonce_flds = {2, bpan2_enonce_fld_list, 16};

reg_decoder_fld_t bpan2_abrctrl1_fld_list[] = {
    {"breaklinktimer",
     15,
     0,
     0,
     11909,
     "Timer for the amount of time to wait in order to assure that the link "
     "partner enters a Link Fail state.  The timer expires  60ms to 75ms after "
     "being started.  Section 73.10.2"},

};
reg_decoder_t bpan2_abrctrl1_flds = {1, bpan2_abrctrl1_fld_list, 16};

reg_decoder_fld_t bpan2_abrctrl2_fld_list[] = {
    {"breaklinktimer",
     15,
     0,
     0,
     11910,
     "Timer for the amount of time to wait in order to assure that the link "
     "partner enters a Link Fail state.  The timer expires  60ms to 75ms after "
     "being started.  Section 73.10.2"},

};
reg_decoder_t bpan2_abrctrl2_flds = {1, bpan2_abrctrl2_fld_list, 16};

reg_decoder_fld_t bpan2_pagetestmaxtimer_fld_list[] = {
    {"pagetestmaxtimer",
     15,
     0,
     0,
     11911,
     "Timer for the maximum time between detection of Manchester violation "
     "delimiters.  This timer is used in conjunction with the "
     "page_test_min_timer to detect whether the link partner is transmitting "
     "DME pages.  The page_test_max_timer expires 350ns to 375ns after being "
     "started or restarted.  Section 73.10.2"},

};
reg_decoder_t bpan2_pagetestmaxtimer_flds = {
    1, bpan2_pagetestmaxtimer_fld_list, 16};

reg_decoder_fld_t bpan2_pagetestmintimer_fld_list[] = {
    {"pagetestmintimer",
     15,
     0,
     0,
     11912,
     "Timer for the minimum time between detection of Manchester violation "
     "delimiters.  This timer is used in conjunction with the "
     "page_test_max_timer to detect whether the link partner is transmitting "
     "DME pages.  The page_test_min_timer expires 305ns to 330ns after being "
     "started or restarted.  Section 73.10.2"},

};
reg_decoder_t bpan2_pagetestmintimer_flds = {
    1, bpan2_pagetestmintimer_fld_list, 16};

reg_decoder_fld_t bpan2_txbasepagelo_fld_list[] = {
    {"txselector", 4, 0, 0, 11913, "Selector field, Section 73.6.1"},

    {"txechoednonce", 9, 5, 0, 11914, "Echoed Nonce Field, Section 73.6.2"},

    {"txpausecap", 11, 10, 0, 11915, "Section 73.6.6."},

    {"txremotefault", 13, 13, 0, 11916, "Section 73.6.7"},

    {"txack", 14, 14, 0, 11917, "Section 73.6.8"},

    {"txnp", 15, 15, 0, 11918, "Section 73.6.9"},

};
reg_decoder_t bpan2_txbasepagelo_flds = {6, bpan2_txbasepagelo_fld_list, 16};

reg_decoder_fld_t bpan2_txbasepagemid_fld_list[] = {
    {"txnonce",
     4,
     0,
     0,
     11919,
     "Transmitter Nonce field, Section 73.6.3. This is a randomly generated "
     "value. "},

    {"txtechability",
     15,
     5,
     0,
     11920,
     "Section 73.6.4 (802.3bj). One Hot encoding for the supporting technology "
     "based on 802.3bj.  Bit0 : 1000BASE-KX Bit1 : 10GBASE-KX4 Bit2 : "
     "10GBASE-KR Bit3 : 40GBASE-KR4 Bit4 : 40GBASE-CR4 Bit5 : 100GBASE-CR10 "
     "Bit6 : 100GBASE-KP4 Bit7 : 100GBASE-KR4 Bit8 : 100GBASE-CR4  Bit 9~15 "
     "are reserved in the 802.3bj spec.  Note: A compile-time parameterized "
     "mask can be used to control support of auto-negotiation when the "
     "auto-negotiation block is instantiated."},

};
reg_decoder_t bpan2_txbasepagemid_flds = {2, bpan2_txbasepagemid_fld_list, 16};

reg_decoder_fld_t bpan2_txbasepagehi_fld_list[] = {
    {"txtechability",
     13,
     0,
     0,
     11921,
     "Corresponds to tech ability [23:16]. Bit 16~23 are reserved in the "
     "802.3bj spec."},

    {"txfecability",
     15,
     14,
     0,
     11922,
     "Section 73.6.5. FEC capability. Bit[1] is to request FEC. 0: No FEC "
     "request. 1: Request FEC on link. Bit[0] is FEC support.  0: No FEC "
     "supported. 1: FEC supported."},

};
reg_decoder_t bpan2_txbasepagehi_flds = {2, bpan2_txbasepagehi_fld_list, 16};

reg_decoder_fld_t bpan2_rxbasepagelo_fld_list[] = {
    {"rxselector", 4, 0, 0, 11923, "Selector field, Section 73.6.1"},

    {"rxechoednonce", 9, 5, 0, 11924, "Echoed Nonce Field, Section 73.6.2"},

    {"rxpausecap", 11, 10, 0, 11925, "Section 73.6.6."},

    {"rxremotefault", 13, 13, 0, 11926, "Section 73.6.7"},

    {"rxack", 14, 14, 0, 11927, "Section 73.6.8"},

    {"rxnp", 15, 15, 0, 11928, "Section 73.6.9"},

};
reg_decoder_t bpan2_rxbasepagelo_flds = {6, bpan2_rxbasepagelo_fld_list, 16};

reg_decoder_fld_t bpan2_rxbasepagemid_fld_list[] = {
    {"rxnonce",
     4,
     0,
     0,
     11929,
     "Transmitter Nonce field, Section 73.6.3. This is a randomly generated "
     "value. "},

    {"rxtechability",
     15,
     5,
     0,
     11930,
     "Section 73.6.4 (802.3bj). One Hot encoding for the supporting technology "
     "based on 802.3bj.  Bit0 : 1000BASE-KX Bit1 : 10GBASE-KX4 Bit2 : "
     "10GBASE-KR Bit3 : 40GBASE-KR4 Bit4 : 40GBASE-CR4 Bit5 : 100GBASE-CR10 "
     "Bit6 : 100GBASE-KP4 Bit7 : 100GBASE-KR4 Bit8 : 100GBASE-CR4  Bit 9~15 "
     "are reserved in the 802.3bj spec.  Note: A compile-time parameterized "
     "mask can be used to control support of auto-negotiation when the "
     "auto-negotiation block is instantiated."},

};
reg_decoder_t bpan2_rxbasepagemid_flds = {2, bpan2_rxbasepagemid_fld_list, 16};

reg_decoder_fld_t bpan2_rxbasepagehi_fld_list[] = {
    {"rxtechability",
     13,
     0,
     0,
     11931,
     "Corresponds to tech ability [23:16]. Bit 16~23 are reserved in the "
     "802.3bj spec."},

    {"rxfecability",
     15,
     14,
     0,
     11932,
     "Section 73.6.5. FEC capability. Bit[1] is to request FEC. 0: No FEC "
     "request. 1: Request FEC on link. Bit[0] is FEC support.  0: No FEC "
     "supported. 1: FEC supported."},

};
reg_decoder_t bpan2_rxbasepagehi_flds = {2, bpan2_rxbasepagehi_fld_list, 16};

reg_decoder_fld_t bpan2_txnextpagelo_fld_list[] = {
    {"txlowercodefield",
     10,
     0,
     0,
     11933,
     "Message code field or Unformatted code field (section 73.7.7.1)"},

    {"txtoggleenable",
     11,
     11,
     0,
     11934,
     "Control updates to tx_toggle and tx_complete 0x0: Disallow updates 0x1: "
     "Allow updates"},

    {"txacknowledge2",
     12,
     12,
     0,
     11935,
     "Acknowledge 2.When the AN_arb_enable is set, hardware will generate the "
     "acknowledge_2 message."},

    {"txmp",
     13,
     13,
     0,
     11936,
     "Message page 0x0: Next page is an unformatted code field 0x1: Next page "
     "is a message code field"},

    {"txacknowledge",
     14,
     14,
     0,
     11937,
     "Acknowledge.When AN_arb_enable is set, hardware will generate the "
     "acknowledge message."},

    {"txnextpage",
     15,
     15,
     0,
     11938,
     "Next page indication 0x0: Next page is not available 0x1: Next page is "
     "available"},

};
reg_decoder_t bpan2_txnextpagelo_flds = {6, bpan2_txnextpagelo_fld_list, 16};

reg_decoder_fld_t bpan2_txnextpagemid_fld_list[] = {
    {"txunformattedcode",
     15,
     0,
     0,
     11939,
     "Unformatted code field lower word.  This field corresponds  to bit 16: "
     "31 in the DME next page"},

};
reg_decoder_t bpan2_txnextpagemid_flds = {1, bpan2_txnextpagemid_fld_list, 16};

reg_decoder_fld_t bpan2_txnextpagehi_fld_list[] = {
    {"txunformattedcode",
     15,
     0,
     0,
     11940,
     "Unformatted code field upper word.  This field corresponds to bit 32: 47 "
     "in the DME next page"},

};
reg_decoder_t bpan2_txnextpagehi_flds = {1, bpan2_txnextpagehi_fld_list, 16};

reg_decoder_fld_t bpan2_rxnextpagelo_fld_list[] = {
    {"rxlowercodefield",
     10,
     0,
     0,
     11941,
     "Message code field or Unformatted code field (section 73.7.7.1)"},

    {"rxtoggleenable",
     11,
     11,
     0,
     11942,
     "Control updates to rx_toggle and rx_complete 0x0: Disallow updates 0x1: "
     "Allow updates"},

    {"rxacknowledge2",
     12,
     12,
     0,
     11943,
     "Acknowledge 2.When the AN_arb_enable is set, hardware will generate the "
     "acknowledge_2 message."},

    {"rxmp",
     13,
     13,
     0,
     11944,
     "Message page 0x0: Next page is an unformatted code field 0x1: Next page "
     "is a message code field"},

    {"rxacknowledge",
     14,
     14,
     0,
     11945,
     "Acknowledge.When AN_arb_enable is set, hardware will generate the "
     "acknowledge message."},

    {"rxnextpage",
     15,
     15,
     0,
     11946,
     "Next page indication 0x0: Next page is not available 0x1: Next page is "
     "available"},

};
reg_decoder_t bpan2_rxnextpagelo_flds = {6, bpan2_rxnextpagelo_fld_list, 16};

reg_decoder_fld_t bpan2_rxnextpagemid_fld_list[] = {
    {"rxunformattedcode",
     15,
     0,
     0,
     11947,
     "Unformatted code field lower word.  This field corresponds  to bit 16: "
     "31 in the DME next page"},

};
reg_decoder_t bpan2_rxnextpagemid_flds = {1, bpan2_rxnextpagemid_fld_list, 16};

reg_decoder_fld_t bpan2_rxnextpagehi_fld_list[] = {
    {"rxunformattedcode",
     15,
     0,
     0,
     11948,
     "Unformatted code field upper word.  This field corresponds to bit 32: 47 "
     "in the DME next page"},

};
reg_decoder_t bpan2_rxnextpagehi_flds = {1, bpan2_rxnextpagehi_fld_list, 16};

reg_decoder_fld_t bpan2_fint0_fld_list[] = {
    {"ovrdint", 5, 0, 0, 11949, "BPAN TX base page Interrupt override"},

};
reg_decoder_t bpan2_fint0_flds = {1, bpan2_fint0_fld_list, 16};

reg_decoder_fld_t bpan2_intstatus_fld_list[] = {
    {"txbp", 0, 0, 0, 11950, "Transmitted Base Page status"},

    {"txnp", 1, 1, 0, 11951, "Transmitted Next Page status"},

    {"txack", 2, 2, 0, 11952, "Transmitted Page Acknowledge status"},

    {"rxbp", 3, 3, 0, 11953, "Received Base Page status"},

    {"rxnp", 4, 4, 0, 11954, "Received Next Page status"},

    {"ancomplete", 5, 5, 0, 11955, "Autonegotiation complete status"},

};
reg_decoder_t bpan2_intstatus_flds = {6, bpan2_intstatus_fld_list, 16};

reg_decoder_fld_t bpan3_ctrl1_fld_list[] = {
    {"ansetmode", 0, 0, 0, 11956, "Set operation mode 0: Idle 1: Normal"},

    {"andisablepn",
     1,
     1,
     0,
     11957,
     "Disable PN sequence of bit 49. 0: enable 1: disable"},

    {"anarbenable",
     2,
     2,
     0,
     11958,
     "Enable the AN arbitration function. 0: disable 1: enable"},

    {"antransmiten",
     8,
     8,
     0,
     11959,
     "Control the enable of the AN transmitter when the AN_arb_enable is "
     "disabled 0: disable 1: enable"},

    {"anrestart", 9, 9, 0, 11960, "Restart autonegotiation"},

    {"anreceiveen",
     10,
     10,
     0,
     11961,
     "Control the enable of the AN receiver when the AN_arb_enable is disabled "
     "0: disable 1: enable"},

    {"anrxgbresync",
     11,
     11,
     0,
     11962,
     "Receive gear box resync, manually force re-initialization ofe the edge "
     "detector for the Receive Gear Box 0: disable 1: enable"},

    {"anenable",
     12,
     12,
     0,
     11963,
     "Enable the AN function.  Mainly used for clock gating of the entire "
     "block. 0: disable 1: enable AN"},

    {"anbasepage",
     13,
     13,
     0,
     11964,
     "Base page transfer enable for software control mode.  When this bit is "
     "set to base page, the base page register will be used for transmission "
     "and reception.  Otherwise, use the next page registers. 0: base page "
     "transfer 1: next page transfer"},

    {"anrxgbresyncen",
     14,
     14,
     0,
     11965,
     "Receive gear box resync, allow hardware controlled re-initialization of "
     "the edge detector for the Receive Gear Box 0: disable 1: enable"},

    {"ansoftrstn", 15, 15, 0, 11966, "Soft reset 0: enable 1: disable"},

};
reg_decoder_t bpan3_ctrl1_flds = {11, bpan3_ctrl1_fld_list, 16};

reg_decoder_fld_t bpan3_status1_fld_list[] = {
    {"lpanenable",
     0,
     0,
     0,
     11967,
     "Link Partner autonegotiationable 0: Negotiation has not begun 1: "
     "Negotiation has begun"},

    {"anability",
     3,
     3,
     0,
     11968,
     "Used to signal that this design has BPAN capabilities"},

    {"ancomplete", 5, 5, 0, 11969, "Autonegotiation complete"},

    {"pagerx", 6, 6, 0, 11970, "Page received"},

    {"nploaded", 10, 10, 0, 11971, "Next page loaded"},

};
reg_decoder_t bpan3_status1_flds = {5, bpan3_status1_fld_list, 16};

reg_decoder_fld_t bpan3_tnonce_fld_list[] = {
    {"antnonceforceval",
     4,
     0,
     0,
     11972,
     "Value forced into the transmitted Nonce Field."},

    {"antnonceforceen",
     5,
     5,
     0,
     11973,
     "Enable the forcing of transmitter Nonce field for debugging purposes. "
     "0:disable 1:enable"},

};
reg_decoder_t bpan3_tnonce_flds = {2, bpan3_tnonce_fld_list, 16};

reg_decoder_fld_t bpan3_enonce_fld_list[] = {
    {"anforceenonceval",
     4,
     0,
     0,
     11974,
     "Section 73.6.2. The forced value for the echo."},

    {"anforceenonceen",
     5,
     5,
     0,
     11975,
     "Section 73.6.2, enable the forcing of echoed nonce value instead of "
     "using echoed value. 0: disable 1: enable"},

};
reg_decoder_t bpan3_enonce_flds = {2, bpan3_enonce_fld_list, 16};

reg_decoder_fld_t bpan3_abrctrl1_fld_list[] = {
    {"breaklinktimer",
     15,
     0,
     0,
     11976,
     "Timer for the amount of time to wait in order to assure that the link "
     "partner enters a Link Fail state.  The timer expires  60ms to 75ms after "
     "being started.  Section 73.10.2"},

};
reg_decoder_t bpan3_abrctrl1_flds = {1, bpan3_abrctrl1_fld_list, 16};

reg_decoder_fld_t bpan3_abrctrl2_fld_list[] = {
    {"breaklinktimer",
     15,
     0,
     0,
     11977,
     "Timer for the amount of time to wait in order to assure that the link "
     "partner enters a Link Fail state.  The timer expires  60ms to 75ms after "
     "being started.  Section 73.10.2"},

};
reg_decoder_t bpan3_abrctrl2_flds = {1, bpan3_abrctrl2_fld_list, 16};

reg_decoder_fld_t bpan3_pagetestmaxtimer_fld_list[] = {
    {"pagetestmaxtimer",
     15,
     0,
     0,
     11978,
     "Timer for the maximum time between detection of Manchester violation "
     "delimiters.  This timer is used in conjunction with the "
     "page_test_min_timer to detect whether the link partner is transmitting "
     "DME pages.  The page_test_max_timer expires 350ns to 375ns after being "
     "started or restarted.  Section 73.10.2"},

};
reg_decoder_t bpan3_pagetestmaxtimer_flds = {
    1, bpan3_pagetestmaxtimer_fld_list, 16};

reg_decoder_fld_t bpan3_pagetestmintimer_fld_list[] = {
    {"pagetestmintimer",
     15,
     0,
     0,
     11979,
     "Timer for the minimum time between detection of Manchester violation "
     "delimiters.  This timer is used in conjunction with the "
     "page_test_max_timer to detect whether the link partner is transmitting "
     "DME pages.  The page_test_min_timer expires 305ns to 330ns after being "
     "started or restarted.  Section 73.10.2"},

};
reg_decoder_t bpan3_pagetestmintimer_flds = {
    1, bpan3_pagetestmintimer_fld_list, 16};

reg_decoder_fld_t bpan3_txbasepagelo_fld_list[] = {
    {"txselector", 4, 0, 0, 11980, "Selector field, Section 73.6.1"},

    {"txechoednonce", 9, 5, 0, 11981, "Echoed Nonce Field, Section 73.6.2"},

    {"txpausecap", 11, 10, 0, 11982, "Section 73.6.6."},

    {"txremotefault", 13, 13, 0, 11983, "Section 73.6.7"},

    {"txack", 14, 14, 0, 11984, "Section 73.6.8"},

    {"txnp", 15, 15, 0, 11985, "Section 73.6.9"},

};
reg_decoder_t bpan3_txbasepagelo_flds = {6, bpan3_txbasepagelo_fld_list, 16};

reg_decoder_fld_t bpan3_txbasepagemid_fld_list[] = {
    {"txnonce",
     4,
     0,
     0,
     11986,
     "Transmitter Nonce field, Section 73.6.3. This is a randomly generated "
     "value. "},

    {"txtechability",
     15,
     5,
     0,
     11987,
     "Section 73.6.4 (802.3bj). One Hot encoding for the supporting technology "
     "based on 802.3bj.  Bit0 : 1000BASE-KX Bit1 : 10GBASE-KX4 Bit2 : "
     "10GBASE-KR Bit3 : 40GBASE-KR4 Bit4 : 40GBASE-CR4 Bit5 : 100GBASE-CR10 "
     "Bit6 : 100GBASE-KP4 Bit7 : 100GBASE-KR4 Bit8 : 100GBASE-CR4  Bit 9~15 "
     "are reserved in the 802.3bj spec.  Note: A compile-time parameterized "
     "mask can be used to control support of auto-negotiation when the "
     "auto-negotiation block is instantiated."},

};
reg_decoder_t bpan3_txbasepagemid_flds = {2, bpan3_txbasepagemid_fld_list, 16};

reg_decoder_fld_t bpan3_txbasepagehi_fld_list[] = {
    {"txtechability",
     13,
     0,
     0,
     11988,
     "Corresponds to tech ability [23:16]. Bit 16~23 are reserved in the "
     "802.3bj spec."},

    {"txfecability",
     15,
     14,
     0,
     11989,
     "Section 73.6.5. FEC capability. Bit[1] is to request FEC. 0: No FEC "
     "request. 1: Request FEC on link. Bit[0] is FEC support.  0: No FEC "
     "supported. 1: FEC supported."},

};
reg_decoder_t bpan3_txbasepagehi_flds = {2, bpan3_txbasepagehi_fld_list, 16};

reg_decoder_fld_t bpan3_rxbasepagelo_fld_list[] = {
    {"rxselector", 4, 0, 0, 11990, "Selector field, Section 73.6.1"},

    {"rxechoednonce", 9, 5, 0, 11991, "Echoed Nonce Field, Section 73.6.2"},

    {"rxpausecap", 11, 10, 0, 11992, "Section 73.6.6."},

    {"rxremotefault", 13, 13, 0, 11993, "Section 73.6.7"},

    {"rxack", 14, 14, 0, 11994, "Section 73.6.8"},

    {"rxnp", 15, 15, 0, 11995, "Section 73.6.9"},

};
reg_decoder_t bpan3_rxbasepagelo_flds = {6, bpan3_rxbasepagelo_fld_list, 16};

reg_decoder_fld_t bpan3_rxbasepagemid_fld_list[] = {
    {"rxnonce",
     4,
     0,
     0,
     11996,
     "Transmitter Nonce field, Section 73.6.3. This is a randomly generated "
     "value. "},

    {"rxtechability",
     15,
     5,
     0,
     11997,
     "Section 73.6.4 (802.3bj). One Hot encoding for the supporting technology "
     "based on 802.3bj.  Bit0 : 1000BASE-KX Bit1 : 10GBASE-KX4 Bit2 : "
     "10GBASE-KR Bit3 : 40GBASE-KR4 Bit4 : 40GBASE-CR4 Bit5 : 100GBASE-CR10 "
     "Bit6 : 100GBASE-KP4 Bit7 : 100GBASE-KR4 Bit8 : 100GBASE-CR4  Bit 9~15 "
     "are reserved in the 802.3bj spec.  Note: A compile-time parameterized "
     "mask can be used to control support of auto-negotiation when the "
     "auto-negotiation block is instantiated."},

};
reg_decoder_t bpan3_rxbasepagemid_flds = {2, bpan3_rxbasepagemid_fld_list, 16};

reg_decoder_fld_t bpan3_rxbasepagehi_fld_list[] = {
    {"rxtechability",
     13,
     0,
     0,
     11998,
     "Corresponds to tech ability [23:16]. Bit 16~23 are reserved in the "
     "802.3bj spec."},

    {"rxfecability",
     15,
     14,
     0,
     11999,
     "Section 73.6.5. FEC capability. Bit[1] is to request FEC. 0: No FEC "
     "request. 1: Request FEC on link. Bit[0] is FEC support.  0: No FEC "
     "supported. 1: FEC supported."},

};
reg_decoder_t bpan3_rxbasepagehi_flds = {2, bpan3_rxbasepagehi_fld_list, 16};

reg_decoder_fld_t bpan3_txnextpagelo_fld_list[] = {
    {"txlowercodefield",
     10,
     0,
     0,
     12000,
     "Message code field or Unformatted code field (section 73.7.7.1)"},

    {"txtoggleenable",
     11,
     11,
     0,
     12001,
     "Control updates to tx_toggle and tx_complete 0x0: Disallow updates 0x1: "
     "Allow updates"},

    {"txacknowledge2",
     12,
     12,
     0,
     12002,
     "Acknowledge 2.When the AN_arb_enable is set, hardware will generate the "
     "acknowledge_2 message."},

    {"txmp",
     13,
     13,
     0,
     12003,
     "Message page 0x0: Next page is an unformatted code field 0x1: Next page "
     "is a message code field"},

    {"txacknowledge",
     14,
     14,
     0,
     12004,
     "Acknowledge.When AN_arb_enable is set, hardware will generate the "
     "acknowledge message."},

    {"txnextpage",
     15,
     15,
     0,
     12005,
     "Next page indication 0x0: Next page is not available 0x1: Next page is "
     "available"},

};
reg_decoder_t bpan3_txnextpagelo_flds = {6, bpan3_txnextpagelo_fld_list, 16};

reg_decoder_fld_t bpan3_txnextpagemid_fld_list[] = {
    {"txunformattedcode",
     15,
     0,
     0,
     12006,
     "Unformatted code field lower word.  This field corresponds  to bit 16: "
     "31 in the DME next page"},

};
reg_decoder_t bpan3_txnextpagemid_flds = {1, bpan3_txnextpagemid_fld_list, 16};

reg_decoder_fld_t bpan3_txnextpagehi_fld_list[] = {
    {"txunformattedcode",
     15,
     0,
     0,
     12007,
     "Unformatted code field upper word.  This field corresponds to bit 32: 47 "
     "in the DME next page"},

};
reg_decoder_t bpan3_txnextpagehi_flds = {1, bpan3_txnextpagehi_fld_list, 16};

reg_decoder_fld_t bpan3_rxnextpagelo_fld_list[] = {
    {"rxlowercodefield",
     10,
     0,
     0,
     12008,
     "Message code field or Unformatted code field (section 73.7.7.1)"},

    {"rxtoggleenable",
     11,
     11,
     0,
     12009,
     "Control updates to rx_toggle and rx_complete 0x0: Disallow updates 0x1: "
     "Allow updates"},

    {"rxacknowledge2",
     12,
     12,
     0,
     12010,
     "Acknowledge 2.When the AN_arb_enable is set, hardware will generate the "
     "acknowledge_2 message."},

    {"rxmp",
     13,
     13,
     0,
     12011,
     "Message page 0x0: Next page is an unformatted code field 0x1: Next page "
     "is a message code field"},

    {"rxacknowledge",
     14,
     14,
     0,
     12012,
     "Acknowledge.When AN_arb_enable is set, hardware will generate the "
     "acknowledge message."},

    {"rxnextpage",
     15,
     15,
     0,
     12013,
     "Next page indication 0x0: Next page is not available 0x1: Next page is "
     "available"},

};
reg_decoder_t bpan3_rxnextpagelo_flds = {6, bpan3_rxnextpagelo_fld_list, 16};

reg_decoder_fld_t bpan3_rxnextpagemid_fld_list[] = {
    {"rxunformattedcode",
     15,
     0,
     0,
     12014,
     "Unformatted code field lower word.  This field corresponds  to bit 16: "
     "31 in the DME next page"},

};
reg_decoder_t bpan3_rxnextpagemid_flds = {1, bpan3_rxnextpagemid_fld_list, 16};

reg_decoder_fld_t bpan3_rxnextpagehi_fld_list[] = {
    {"rxunformattedcode",
     15,
     0,
     0,
     12015,
     "Unformatted code field upper word.  This field corresponds to bit 32: 47 "
     "in the DME next page"},

};
reg_decoder_t bpan3_rxnextpagehi_flds = {1, bpan3_rxnextpagehi_fld_list, 16};

reg_decoder_fld_t bpan3_fint0_fld_list[] = {
    {"ovrdint", 5, 0, 0, 12016, "BPAN TX base page Interrupt override"},

};
reg_decoder_t bpan3_fint0_flds = {1, bpan3_fint0_fld_list, 16};

reg_decoder_fld_t bpan3_intstatus_fld_list[] = {
    {"txbp", 0, 0, 0, 12017, "Transmitted Base Page status"},

    {"txnp", 1, 1, 0, 12018, "Transmitted Next Page status"},

    {"txack", 2, 2, 0, 12019, "Transmitted Page Acknowledge status"},

    {"rxbp", 3, 3, 0, 12020, "Received Base Page status"},

    {"rxnp", 4, 4, 0, 12021, "Received Next Page status"},

    {"ancomplete", 5, 5, 0, 12022, "Autonegotiation complete status"},

};
reg_decoder_t bpan3_intstatus_flds = {6, bpan3_intstatus_fld_list, 16};

reg_decoder_fld_t serdesmux_serdessigokovrd_fld_list[] = {
    {"chsigstatovrd0",
     1,
     0,
     0,
     12023,
     "Controls Serdes#0 Signal OK signal. For diagnostics only 2'b00: Pass the "
     "signal from SERDES as is 2'b01: Inverted 2'b10: Force the SERDES OK to "
     "1'b0 going into PCS 2'b11: Force the SERDES OK to 1'b1 going into PCS"},

    {"chsigstatovrd1",
     3,
     2,
     0,
     12024,
     "Controls Serdes#1 Signal OK signal. For diagnostics only 2'b00: Pass the "
     "signal from SERDES as is 2'b01: Inverted 2'b10: Force the SERDES OK to "
     "1'b0 going into PCS 2'b11: Force the SERDES OK to 1'b1 going into PCS"},

    {"chsigstatovrd2",
     5,
     4,
     0,
     12025,
     "Controls Serdes#2 Signal OK signal. For diagnostics only 2'b00: Pass the "
     "signal from SERDES as is 2'b01: Inverted 2'b10: Force the SERDES OK to "
     "1'b0 going into PCS 2'b11: Force the SERDES OK to 1'b1 going into PCS"},

    {"chsigstatovrd3",
     7,
     6,
     0,
     12026,
     "Controls Serdes#3 Signal OK signal. For diagnostics only 2'b00: Pass the "
     "signal from SERDES as is 2'b01: Inverted 2'b10: Force the SERDES OK to "
     "1'b0 going into PCS 2'b11: Force the SERDES OK to 1'b1 going into PCS"},

};
reg_decoder_t serdesmux_serdessigokovrd_flds = {
    4, serdesmux_serdessigokovrd_fld_list, 16};

reg_decoder_fld_t serdesmux_serdeslpbk_fld_list[] = {
    {"lpbken0",
     0,
     0,
     0,
     12027,
     "Controls Channel Interface #0 Loopback 1'b0: Normal 1'b1: Loopback "
     "Enabled"},

    {"lpbken1",
     1,
     1,
     0,
     12028,
     "Controls Channel Interface #1 Loopback 1'b0: Normal 1'b1: Loopback "
     "Enabled"},

    {"lpbken2",
     2,
     2,
     0,
     12029,
     "Controls Channel Interface #2 Loopback 1'b0: Normal 1'b1: Loopback "
     "Enabled"},

    {"lpbken3",
     3,
     3,
     0,
     12030,
     "Controls Channel Interface #3 Loopback 1'b0: Normal 1'b1: Loopback "
     "Enabled"},

};
reg_decoder_t serdesmux_serdeslpbk_flds = {
    4, serdesmux_serdeslpbk_fld_list, 16};

reg_decoder_fld_t serdesmux_laneremaprx0_fld_list[] = {
    {"remap0",
     2,
     0,
     0,
     12031,
     "SerDes lane that is connected to PCS RX lane#0"},

    {"remap1",
     5,
     3,
     0,
     12032,
     "SerDes lane that is connected to PCS RX lane#1"},

    {"remap2",
     8,
     6,
     0,
     12033,
     "SerDes lane that is connected to PCS RX lane#2"},

    {"remap3",
     11,
     9,
     0,
     12034,
     "SerDes lane that is connected to PCS RX lane#3"},

};
reg_decoder_t serdesmux_laneremaprx0_flds = {
    4, serdesmux_laneremaprx0_fld_list, 16};

reg_decoder_fld_t serdesmux_laneremaptx0_fld_list[] = {
    {"remap0",
     2,
     0,
     0,
     12035,
     "SerDes lane that is connected to PCS TX lane#0"},

    {"remap1",
     5,
     3,
     0,
     12036,
     "SerDes lane that is connected to PCS TX lane#1"},

    {"remap2",
     8,
     6,
     0,
     12037,
     "SerDes lane that is connected to PCS TX lane#2"},

    {"remap3",
     11,
     9,
     0,
     12038,
     "SerDes lane that is connected to PCS TX lane#3"},

};
reg_decoder_t serdesmux_laneremaptx0_flds = {
    4, serdesmux_laneremaptx0_fld_list, 16};

reg_decoder_fld_t serdesmux_fifosts0_fld_list[] = {
    {"txffwlvl", 4, 0, 0, 12039, "TX FIFO write fill level"},

    {"txffrlvl", 12, 8, 0, 12040, "TX FIFO read fill level"},

};
reg_decoder_t serdesmux_fifosts0_flds = {2, serdesmux_fifosts0_fld_list, 16};

reg_decoder_fld_t serdesmux_fifosts1_fld_list[] = {
    {"rxffwlvl", 3, 0, 0, 12041, "RX FIFO write fill level"},

    {"rxffrlvl", 11, 8, 0, 12042, "RX FIFO read fill level"},

};
reg_decoder_t serdesmux_fifosts1_flds = {2, serdesmux_fifosts1_fld_list, 16};

reg_decoder_fld_t serdesmux_fifosts2_fld_list[] = {
    {"txffwlvl", 4, 0, 0, 12043, "TX FIFO write fill level"},

    {"txffrlvl", 12, 8, 0, 12044, "TX FIFO read fill level"},

};
reg_decoder_t serdesmux_fifosts2_flds = {2, serdesmux_fifosts2_fld_list, 16};

reg_decoder_fld_t serdesmux_fifosts3_fld_list[] = {
    {"rxffwlvl", 3, 0, 0, 12045, "RX FIFO write fill level"},

    {"rxffrlvl", 11, 8, 0, 12046, "RX FIFO read fill level"},

};
reg_decoder_t serdesmux_fifosts3_flds = {2, serdesmux_fifosts3_fld_list, 16};

reg_decoder_fld_t serdesmux_fifosts4_fld_list[] = {
    {"txffwlvl", 4, 0, 0, 12047, "TX FIFO write fill level"},

    {"txffrlvl", 12, 8, 0, 12048, "TX FIFO read fill level"},

};
reg_decoder_t serdesmux_fifosts4_flds = {2, serdesmux_fifosts4_fld_list, 16};

reg_decoder_fld_t serdesmux_fifosts5_fld_list[] = {
    {"rxffwlvl", 3, 0, 0, 12049, "RX FIFO write fill level"},

    {"rxffrlvl", 11, 8, 0, 12050, "RX FIFO read fill level"},

};
reg_decoder_t serdesmux_fifosts5_flds = {2, serdesmux_fifosts5_fld_list, 16};

reg_decoder_fld_t serdesmux_fifosts6_fld_list[] = {
    {"txffwlvl", 4, 0, 0, 12051, "TX FIFO write fill level"},

    {"txffrlvl", 12, 8, 0, 12052, "TX FIFO read fill level"},

};
reg_decoder_t serdesmux_fifosts6_flds = {2, serdesmux_fifosts6_fld_list, 16};

reg_decoder_fld_t serdesmux_fifosts7_fld_list[] = {
    {"rxffwlvl", 3, 0, 0, 12053, "RX FIFO write fill level"},

    {"rxffrlvl", 11, 8, 0, 12054, "RX FIFO read fill level"},

};
reg_decoder_t serdesmux_fifosts7_flds = {2, serdesmux_fifosts7_fld_list, 16};

reg_decoder_fld_t serdesmux_fifoerrsts_fld_list[] = {
    {"rxoverflow0", 0, 0, 0, 12055, "RX FIFO Overflow of Ln#0"},

    {"txunderflow0", 1, 1, 0, 12056, "TX FIFO Underflow of Ln#0"},

    {"rxoverflow1", 2, 2, 0, 12057, "RX FIFO Overflow of Ln#1"},

    {"txunderflow1", 3, 3, 0, 12058, "TX FIFO Underflow of Ln#1"},

    {"rxoverflow2", 4, 4, 0, 12059, "RX FIFO Overflow of Ln#2"},

    {"txunderflow2", 5, 5, 0, 12060, "TX FIFO Underflow of Ln#2"},

    {"rxoverflow3", 6, 6, 0, 12061, "RX FIFO Overflow of Ln#3"},

    {"txunderflow3", 7, 7, 0, 12062, "TX FIFO Underflow of Ln#3"},

};
reg_decoder_t serdesmux_fifoerrsts_flds = {
    8, serdesmux_fifoerrsts_fld_list, 16};

cmd_arg_item_t interrupts0_regs[] = {
    {"setintenable0", NULL, 0x0 * 4, &interrupts0_setintenable0_flds},
    {"clrintenable0", NULL, 0x1 * 4, &interrupts0_clrintenable0_flds},
    {"setintenable1", NULL, 0x2 * 4, &interrupts0_setintenable1_flds},
    {"clrintenable1", NULL, 0x3 * 4, &interrupts0_clrintenable1_flds},
    {"setintenable2", NULL, 0x4 * 4, &interrupts0_setintenable2_flds},
    {"clrintenable2", NULL, 0x5 * 4, &interrupts0_clrintenable2_flds},
    {"setintenable3", NULL, 0x6 * 4, &interrupts0_setintenable3_flds},
    {"clrintenable3", NULL, 0x7 * 4, &interrupts0_clrintenable3_flds},
    {"setintenable4", NULL, 0x8 * 4, &interrupts0_setintenable4_flds},
    {"clrintenable4", NULL, 0x9 * 4, &interrupts0_clrintenable4_flds},
    {"setintenable5", NULL, 0xa * 4, &interrupts0_setintenable5_flds},
    {"clrintenable5", NULL, 0xb * 4, &interrupts0_clrintenable5_flds},
    {"setintenable6", NULL, 0xc * 4, &interrupts0_setintenable6_flds},
    {"clrintenable6", NULL, 0xd * 4, &interrupts0_clrintenable6_flds},
    {"setintenable7", NULL, 0xe * 4, &interrupts0_setintenable7_flds},
    {"clrintenable7", NULL, 0xf * 4, &interrupts0_clrintenable7_flds},
    {"setintenable8", NULL, 0x10 * 4, &interrupts0_setintenable8_flds},
    {"clrintenable8", NULL, 0x11 * 4, &interrupts0_clrintenable8_flds},
    {"setintenable9", NULL, 0x12 * 4, &interrupts0_setintenable9_flds},
    {"clrintenable9", NULL, 0x13 * 4, &interrupts0_clrintenable9_flds},
    {"setintenable10", NULL, 0x14 * 4, &interrupts0_setintenable10_flds},
    {"clrintenable10", NULL, 0x15 * 4, &interrupts0_clrintenable10_flds},
    {"setintenable11", NULL, 0x16 * 4, &interrupts0_setintenable11_flds},
    {"clrintenable11", NULL, 0x17 * 4, &interrupts0_clrintenable11_flds},
    {"setintenable12", NULL, 0x18 * 4, &interrupts0_setintenable12_flds},
    {"clrintenable12", NULL, 0x19 * 4, &interrupts0_clrintenable12_flds},
    {"setintenable13", NULL, 0x1a * 4, &interrupts0_setintenable13_flds},
    {"clrintenable13", NULL, 0x1b * 4, &interrupts0_clrintenable13_flds},
    {"setintenable14", NULL, 0x1c * 4, &interrupts0_setintenable14_flds},
    {"clrintenable14", NULL, 0x1d * 4, &interrupts0_clrintenable14_flds},
    {"setintenable15", NULL, 0x1e * 4, &interrupts0_setintenable15_flds},
    {"clrintenable15", NULL, 0x1f * 4, &interrupts0_clrintenable15_flds},
    {"intstat0", NULL, 0x20 * 4, &interrupts0_intstat0_flds},
    {"intclr0", NULL, 0x21 * 4, &interrupts0_intclr0_flds},
    {"intstat1", NULL, 0x22 * 4, &interrupts0_intstat1_flds},
    {"intclr1", NULL, 0x23 * 4, &interrupts0_intclr1_flds},
    {"intstat2", NULL, 0x24 * 4, &interrupts0_intstat2_flds},
    {"intclr2", NULL, 0x25 * 4, &interrupts0_intclr2_flds},
    {"intstat3", NULL, 0x26 * 4, &interrupts0_intstat3_flds},
    {"intclr3", NULL, 0x27 * 4, &interrupts0_intclr3_flds},
    {"intstat4", NULL, 0x28 * 4, &interrupts0_intstat4_flds},
    {"intclr4", NULL, 0x29 * 4, &interrupts0_intclr4_flds},
    {"intstat5", NULL, 0x2a * 4, &interrupts0_intstat5_flds},
    {"intclr5", NULL, 0x2b * 4, &interrupts0_intclr5_flds},
    {"intstat6", NULL, 0x2c * 4, &interrupts0_intstat6_flds},
    {"intclr6", NULL, 0x2d * 4, &interrupts0_intclr6_flds},
    {"intstat7", NULL, 0x2e * 4, &interrupts0_intstat7_flds},
    {"intclr7", NULL, 0x2f * 4, &interrupts0_intclr7_flds},
    {"intstat8", NULL, 0x30 * 4, &interrupts0_intstat8_flds},
    {"intclr8", NULL, 0x31 * 4, &interrupts0_intclr8_flds},
    {"intstat9", NULL, 0x32 * 4, &interrupts0_intstat9_flds},
    {"intclr9", NULL, 0x33 * 4, &interrupts0_intclr9_flds},
    {"intstat10", NULL, 0x34 * 4, &interrupts0_intstat10_flds},
    {"intclr10", NULL, 0x35 * 4, &interrupts0_intclr10_flds},
    {"intstat11", NULL, 0x36 * 4, &interrupts0_intstat11_flds},
    {"intclr11", NULL, 0x37 * 4, &interrupts0_intclr11_flds},
    {"intstat12", NULL, 0x38 * 4, &interrupts0_intstat12_flds},
    {"intclr12", NULL, 0x39 * 4, &interrupts0_intclr12_flds},
    {"intstat13", NULL, 0x3a * 4, &interrupts0_intstat13_flds},
    {"intclr13", NULL, 0x3b * 4, &interrupts0_intclr13_flds},
    {"intstat14", NULL, 0x3c * 4, &interrupts0_intstat14_flds},
    {"intclr14", NULL, 0x3d * 4, &interrupts0_intclr14_flds},
    {"intstat15", NULL, 0x3e * 4, &interrupts0_intstat15_flds},
    {"intclr15", NULL, 0x3f * 4, &interrupts0_intclr15_flds},
};
cmd_arg_t interrupts0_list = {64, interrupts0_regs};

cmd_arg_item_t glbl_regs[] = {
    {"version", NULL, 0x0 * 4, &glbl_version_flds},
    {"lnkstatovrd", NULL, 0x8 * 4, &glbl_lnkstatovrd_flds},
    {"livelnkstat0", NULL, 0x9 * 4, &glbl_livelnkstat0_flds},
    {"spare0", NULL, 0xb * 4, &glbl_spare0_flds},
    {"spare2", NULL, 0xd * 4, &glbl_spare2_flds},
    {"ch0mode", NULL, 0x10 * 4, &glbl_ch0mode_flds},
    {"ch1mode", NULL, 0x11 * 4, &glbl_ch1mode_flds},
    {"ch2mode", NULL, 0x12 * 4, &glbl_ch2mode_flds},
    {"ch3mode", NULL, 0x13 * 4, &glbl_ch3mode_flds},
    {"fint0", NULL, 0x70 * 4, &glbl_fint0_flds},
    {"defineid", NULL, 0xfe * 4, &glbl_defineid_flds},
    {"productid", NULL, 0xff * 4, &glbl_productid_flds},
};
cmd_arg_t glbl_list = {12, glbl_regs};

cmd_arg_item_t stats0_regs[] = {
    {"swreset", NULL, 0x0 * 4, &stats0_swreset_flds},
    {"rdctrl", NULL, 0x1 * 4, &stats0_rdctrl_flds},
    {"rdata0", NULL, 0x2 * 4, &stats0_rdata0_flds},
    {"rdata1", NULL, 0x3 * 4, &stats0_rdata1_flds},
    {"rdata2", NULL, 0x4 * 4, &stats0_rdata2_flds},
    {"rdata3", NULL, 0x5 * 4, &stats0_rdata3_flds},
    {"statsrst", NULL, 0x6 * 4, &stats0_statsrst_flds},
    {"ledinfo0", NULL, 0x8 * 4, &stats0_ledinfo0_flds},
    {"ledinfo1", NULL, 0x9 * 4, &stats0_ledinfo1_flds},
    {"ledinfo2", NULL, 0xa * 4, &stats0_ledinfo2_flds},
    {"ledinfo3", NULL, 0xb * 4, &stats0_ledinfo3_flds},
    {"sts", NULL, 0xc * 4, &stats0_sts_flds},
};
cmd_arg_t stats0_list = {12, stats0_regs};

cmd_arg_item_t mcmac0_regs[] = {
    {"ctrl", NULL, 0x0 * 4, &mcmac0_ctrl_flds},
    {"txconfig", NULL, 0x1 * 4, &mcmac0_txconfig_flds},
    {"rxconfig", NULL, 0x2 * 4, &mcmac0_rxconfig_flds},
    {"maxfrmsize", NULL, 0x3 * 4, &mcmac0_maxfrmsize_flds},
    {"maxtxjabsize", NULL, 0x4 * 4, &mcmac0_maxtxjabsize_flds},
    {"maxrxjabsize", NULL, 0x5 * 4, &mcmac0_maxrxjabsize_flds},
    {"txpfcvec", NULL, 0x6 * 4, &mcmac0_txpfcvec_flds},
    {"vlantag1", NULL, 0x7 * 4, &mcmac0_vlantag1_flds},
    {"vlantag2", NULL, 0x8 * 4, &mcmac0_vlantag2_flds},
    {"vlantag3", NULL, 0x9 * 4, &mcmac0_vlantag3_flds},
    {"macaddrlo", NULL, 0xa * 4, &mcmac0_macaddrlo_flds},
    {"macaddrmid", NULL, 0xb * 4, &mcmac0_macaddrmid_flds},
    {"macaddrhi", NULL, 0xc * 4, &mcmac0_macaddrhi_flds},
    {"mchasht1", NULL, 0xd * 4, &mcmac0_mchasht1_flds},
    {"mchasht2", NULL, 0xe * 4, &mcmac0_mchasht2_flds},
    {"mchasht3", NULL, 0xf * 4, &mcmac0_mchasht3_flds},
    {"mchasht4", NULL, 0x10 * 4, &mcmac0_mchasht4_flds},
    {"fcfrmgen", NULL, 0x11 * 4, &mcmac0_fcfrmgen_flds},
    {"fcdaddrlo", NULL, 0x12 * 4, &mcmac0_fcdaddrlo_flds},
    {"fcdaddrmid", NULL, 0x13 * 4, &mcmac0_fcdaddrmid_flds},
    {"fcdaddrhi", NULL, 0x14 * 4, &mcmac0_fcdaddrhi_flds},
    {"fcsaddrlo", NULL, 0x15 * 4, &mcmac0_fcsaddrlo_flds},
    {"fcsaddrmid", NULL, 0x16 * 4, &mcmac0_fcsaddrmid_flds},
    {"fcsaddrhi", NULL, 0x17 * 4, &mcmac0_fcsaddrhi_flds},
    {"fcpausetime", NULL, 0x18 * 4, &mcmac0_fcpausetime_flds},
    {"xoffpausetime", NULL, 0x19 * 4, &mcmac0_xoffpausetime_flds},
    {"xonpausetime", NULL, 0x1a * 4, &mcmac0_xonpausetime_flds},
    {"txtsinfo", NULL, 0x1b * 4, &mcmac0_txtsinfo_flds},
    {"tsv0", NULL, 0x1c * 4, &mcmac0_tsv0_flds},
    {"tsv1", NULL, 0x1d * 4, &mcmac0_tsv1_flds},
    {"tsv2", NULL, 0x1e * 4, &mcmac0_tsv2_flds},
    {"tsv3", NULL, 0x1f * 4, &mcmac0_tsv3_flds},
    {"txtsdelta", NULL, 0x20 * 4, &mcmac0_txtsdelta_flds},
    {"rxtsdelta", NULL, 0x21 * 4, &mcmac0_rxtsdelta_flds},
    {"minframesize", NULL, 0x23 * 4, &mcmac0_minframesize_flds},
    {"txvlantag", NULL, 0x24 * 4, &mcmac0_txvlantag_flds},
    {"fint0", NULL, 0x70 * 4, &mcmac0_fint0_flds},
    {"fint1", NULL, 0x71 * 4, &mcmac0_fint1_flds},
    {"fint2", NULL, 0x72 * 4, &mcmac0_fint2_flds},
    {"slotclkcnt", NULL, 0x8a * 4, &mcmac0_slotclkcnt_flds},
    {"txdebug", NULL, 0x91 * 4, &mcmac0_txdebug_flds},
    {"spare0", NULL, 0xfe * 4, &mcmac0_spare0_flds},
    {"useccntr", NULL, 0xff * 4, &mcmac0_useccntr_flds},
};
cmd_arg_t mcmac0_list = {43, mcmac0_regs};

cmd_arg_item_t mcmac1_regs[] = {
    {"ctrl", NULL, 0x0 * 4, &mcmac1_ctrl_flds},
    {"txconfig", NULL, 0x1 * 4, &mcmac1_txconfig_flds},
    {"rxconfig", NULL, 0x2 * 4, &mcmac1_rxconfig_flds},
    {"maxfrmsize", NULL, 0x3 * 4, &mcmac1_maxfrmsize_flds},
    {"maxtxjabsize", NULL, 0x4 * 4, &mcmac1_maxtxjabsize_flds},
    {"maxrxjabsize", NULL, 0x5 * 4, &mcmac1_maxrxjabsize_flds},
    {"txpfcvec", NULL, 0x6 * 4, &mcmac1_txpfcvec_flds},
    {"vlantag1", NULL, 0x7 * 4, &mcmac1_vlantag1_flds},
    {"vlantag2", NULL, 0x8 * 4, &mcmac1_vlantag2_flds},
    {"vlantag3", NULL, 0x9 * 4, &mcmac1_vlantag3_flds},
    {"macaddrlo", NULL, 0xa * 4, &mcmac1_macaddrlo_flds},
    {"macaddrmid", NULL, 0xb * 4, &mcmac1_macaddrmid_flds},
    {"macaddrhi", NULL, 0xc * 4, &mcmac1_macaddrhi_flds},
    {"mchasht1", NULL, 0xd * 4, &mcmac1_mchasht1_flds},
    {"mchasht2", NULL, 0xe * 4, &mcmac1_mchasht2_flds},
    {"mchasht3", NULL, 0xf * 4, &mcmac1_mchasht3_flds},
    {"mchasht4", NULL, 0x10 * 4, &mcmac1_mchasht4_flds},
    {"fcfrmgen", NULL, 0x11 * 4, &mcmac1_fcfrmgen_flds},
    {"fcdaddrlo", NULL, 0x12 * 4, &mcmac1_fcdaddrlo_flds},
    {"fcdaddrmid", NULL, 0x13 * 4, &mcmac1_fcdaddrmid_flds},
    {"fcdaddrhi", NULL, 0x14 * 4, &mcmac1_fcdaddrhi_flds},
    {"fcsaddrlo", NULL, 0x15 * 4, &mcmac1_fcsaddrlo_flds},
    {"fcsaddrmid", NULL, 0x16 * 4, &mcmac1_fcsaddrmid_flds},
    {"fcsaddrhi", NULL, 0x17 * 4, &mcmac1_fcsaddrhi_flds},
    {"fcpausetime", NULL, 0x18 * 4, &mcmac1_fcpausetime_flds},
    {"xoffpausetime", NULL, 0x19 * 4, &mcmac1_xoffpausetime_flds},
    {"xonpausetime", NULL, 0x1a * 4, &mcmac1_xonpausetime_flds},
    {"txtsinfo", NULL, 0x1b * 4, &mcmac1_txtsinfo_flds},
    {"tsv0", NULL, 0x1c * 4, &mcmac1_tsv0_flds},
    {"tsv1", NULL, 0x1d * 4, &mcmac1_tsv1_flds},
    {"tsv2", NULL, 0x1e * 4, &mcmac1_tsv2_flds},
    {"tsv3", NULL, 0x1f * 4, &mcmac1_tsv3_flds},
    {"txtsdelta", NULL, 0x20 * 4, &mcmac1_txtsdelta_flds},
    {"rxtsdelta", NULL, 0x21 * 4, &mcmac1_rxtsdelta_flds},
    {"minframesize", NULL, 0x23 * 4, &mcmac1_minframesize_flds},
    {"txvlantag", NULL, 0x24 * 4, &mcmac1_txvlantag_flds},
    {"fint0", NULL, 0x70 * 4, &mcmac1_fint0_flds},
    {"fint1", NULL, 0x71 * 4, &mcmac1_fint1_flds},
    {"fint2", NULL, 0x72 * 4, &mcmac1_fint2_flds},
    {"slotclkcnt", NULL, 0x8a * 4, &mcmac1_slotclkcnt_flds},
    {"txdebug", NULL, 0x91 * 4, &mcmac1_txdebug_flds},
};
cmd_arg_t mcmac1_list = {41, mcmac1_regs};

cmd_arg_item_t mcmac2_regs[] = {
    {"ctrl", NULL, 0x0 * 4, &mcmac2_ctrl_flds},
    {"txconfig", NULL, 0x1 * 4, &mcmac2_txconfig_flds},
    {"rxconfig", NULL, 0x2 * 4, &mcmac2_rxconfig_flds},
    {"maxfrmsize", NULL, 0x3 * 4, &mcmac2_maxfrmsize_flds},
    {"maxtxjabsize", NULL, 0x4 * 4, &mcmac2_maxtxjabsize_flds},
    {"maxrxjabsize", NULL, 0x5 * 4, &mcmac2_maxrxjabsize_flds},
    {"txpfcvec", NULL, 0x6 * 4, &mcmac2_txpfcvec_flds},
    {"vlantag1", NULL, 0x7 * 4, &mcmac2_vlantag1_flds},
    {"vlantag2", NULL, 0x8 * 4, &mcmac2_vlantag2_flds},
    {"vlantag3", NULL, 0x9 * 4, &mcmac2_vlantag3_flds},
    {"macaddrlo", NULL, 0xa * 4, &mcmac2_macaddrlo_flds},
    {"macaddrmid", NULL, 0xb * 4, &mcmac2_macaddrmid_flds},
    {"macaddrhi", NULL, 0xc * 4, &mcmac2_macaddrhi_flds},
    {"mchasht1", NULL, 0xd * 4, &mcmac2_mchasht1_flds},
    {"mchasht2", NULL, 0xe * 4, &mcmac2_mchasht2_flds},
    {"mchasht3", NULL, 0xf * 4, &mcmac2_mchasht3_flds},
    {"mchasht4", NULL, 0x10 * 4, &mcmac2_mchasht4_flds},
    {"fcfrmgen", NULL, 0x11 * 4, &mcmac2_fcfrmgen_flds},
    {"fcdaddrlo", NULL, 0x12 * 4, &mcmac2_fcdaddrlo_flds},
    {"fcdaddrmid", NULL, 0x13 * 4, &mcmac2_fcdaddrmid_flds},
    {"fcdaddrhi", NULL, 0x14 * 4, &mcmac2_fcdaddrhi_flds},
    {"fcsaddrlo", NULL, 0x15 * 4, &mcmac2_fcsaddrlo_flds},
    {"fcsaddrmid", NULL, 0x16 * 4, &mcmac2_fcsaddrmid_flds},
    {"fcsaddrhi", NULL, 0x17 * 4, &mcmac2_fcsaddrhi_flds},
    {"fcpausetime", NULL, 0x18 * 4, &mcmac2_fcpausetime_flds},
    {"xoffpausetime", NULL, 0x19 * 4, &mcmac2_xoffpausetime_flds},
    {"xonpausetime", NULL, 0x1a * 4, &mcmac2_xonpausetime_flds},
    {"txtsinfo", NULL, 0x1b * 4, &mcmac2_txtsinfo_flds},
    {"tsv0", NULL, 0x1c * 4, &mcmac2_tsv0_flds},
    {"tsv1", NULL, 0x1d * 4, &mcmac2_tsv1_flds},
    {"tsv2", NULL, 0x1e * 4, &mcmac2_tsv2_flds},
    {"tsv3", NULL, 0x1f * 4, &mcmac2_tsv3_flds},
    {"txtsdelta", NULL, 0x20 * 4, &mcmac2_txtsdelta_flds},
    {"rxtsdelta", NULL, 0x21 * 4, &mcmac2_rxtsdelta_flds},
    {"minframesize", NULL, 0x23 * 4, &mcmac2_minframesize_flds},
    {"txvlantag", NULL, 0x24 * 4, &mcmac2_txvlantag_flds},
    {"fint0", NULL, 0x70 * 4, &mcmac2_fint0_flds},
    {"fint1", NULL, 0x71 * 4, &mcmac2_fint1_flds},
    {"fint2", NULL, 0x72 * 4, &mcmac2_fint2_flds},
    {"slotclkcnt", NULL, 0x8a * 4, &mcmac2_slotclkcnt_flds},
    {"txdebug", NULL, 0x91 * 4, &mcmac2_txdebug_flds},
};
cmd_arg_t mcmac2_list = {41, mcmac2_regs};

cmd_arg_item_t mcmac3_regs[] = {
    {"ctrl", NULL, 0x0 * 4, &mcmac3_ctrl_flds},
    {"txconfig", NULL, 0x1 * 4, &mcmac3_txconfig_flds},
    {"rxconfig", NULL, 0x2 * 4, &mcmac3_rxconfig_flds},
    {"maxfrmsize", NULL, 0x3 * 4, &mcmac3_maxfrmsize_flds},
    {"maxtxjabsize", NULL, 0x4 * 4, &mcmac3_maxtxjabsize_flds},
    {"maxrxjabsize", NULL, 0x5 * 4, &mcmac3_maxrxjabsize_flds},
    {"txpfcvec", NULL, 0x6 * 4, &mcmac3_txpfcvec_flds},
    {"vlantag1", NULL, 0x7 * 4, &mcmac3_vlantag1_flds},
    {"vlantag2", NULL, 0x8 * 4, &mcmac3_vlantag2_flds},
    {"vlantag3", NULL, 0x9 * 4, &mcmac3_vlantag3_flds},
    {"macaddrlo", NULL, 0xa * 4, &mcmac3_macaddrlo_flds},
    {"macaddrmid", NULL, 0xb * 4, &mcmac3_macaddrmid_flds},
    {"macaddrhi", NULL, 0xc * 4, &mcmac3_macaddrhi_flds},
    {"mchasht1", NULL, 0xd * 4, &mcmac3_mchasht1_flds},
    {"mchasht2", NULL, 0xe * 4, &mcmac3_mchasht2_flds},
    {"mchasht3", NULL, 0xf * 4, &mcmac3_mchasht3_flds},
    {"mchasht4", NULL, 0x10 * 4, &mcmac3_mchasht4_flds},
    {"fcfrmgen", NULL, 0x11 * 4, &mcmac3_fcfrmgen_flds},
    {"fcdaddrlo", NULL, 0x12 * 4, &mcmac3_fcdaddrlo_flds},
    {"fcdaddrmid", NULL, 0x13 * 4, &mcmac3_fcdaddrmid_flds},
    {"fcdaddrhi", NULL, 0x14 * 4, &mcmac3_fcdaddrhi_flds},
    {"fcsaddrlo", NULL, 0x15 * 4, &mcmac3_fcsaddrlo_flds},
    {"fcsaddrmid", NULL, 0x16 * 4, &mcmac3_fcsaddrmid_flds},
    {"fcsaddrhi", NULL, 0x17 * 4, &mcmac3_fcsaddrhi_flds},
    {"fcpausetime", NULL, 0x18 * 4, &mcmac3_fcpausetime_flds},
    {"xoffpausetime", NULL, 0x19 * 4, &mcmac3_xoffpausetime_flds},
    {"xonpausetime", NULL, 0x1a * 4, &mcmac3_xonpausetime_flds},
    {"txtsinfo", NULL, 0x1b * 4, &mcmac3_txtsinfo_flds},
    {"tsv0", NULL, 0x1c * 4, &mcmac3_tsv0_flds},
    {"tsv1", NULL, 0x1d * 4, &mcmac3_tsv1_flds},
    {"tsv2", NULL, 0x1e * 4, &mcmac3_tsv2_flds},
    {"tsv3", NULL, 0x1f * 4, &mcmac3_tsv3_flds},
    {"txtsdelta", NULL, 0x20 * 4, &mcmac3_txtsdelta_flds},
    {"rxtsdelta", NULL, 0x21 * 4, &mcmac3_rxtsdelta_flds},
    {"minframesize", NULL, 0x23 * 4, &mcmac3_minframesize_flds},
    {"txvlantag", NULL, 0x24 * 4, &mcmac3_txvlantag_flds},
    {"fint0", NULL, 0x70 * 4, &mcmac3_fint0_flds},
    {"fint1", NULL, 0x71 * 4, &mcmac3_fint1_flds},
    {"fint2", NULL, 0x72 * 4, &mcmac3_fint2_flds},
    {"slotclkcnt", NULL, 0x8a * 4, &mcmac3_slotclkcnt_flds},
    {"txdebug", NULL, 0x91 * 4, &mcmac3_txdebug_flds},
};
cmd_arg_t mcmac3_list = {41, mcmac3_regs};

cmd_arg_item_t fifoctrl0_regs[] = {
    {"ctrl1", NULL, 0x1 * 4, &fifoctrl0_ctrl1_flds},
    {"ctrl2", NULL, 0x19 * 4, &fifoctrl0_ctrl2_flds},
    {"appfifolpbk", NULL, 0x2 * 4, &fifoctrl0_appfifolpbk_flds},
    {"appfifolpthrsh", NULL, 0x4 * 4, &fifoctrl0_appfifolpthrsh_flds},
    {"appfifoportmap0", NULL, 0x5 * 4, &fifoctrl0_appfifoportmap0_flds},
    {"rxfifoctrl0", NULL, 0x7 * 4, &fifoctrl0_rxfifoctrl0_flds},
    {"rxfifoctrl1", NULL, 0x8 * 4, &fifoctrl0_rxfifoctrl1_flds},
    {"rxfifoctrl2", NULL, 0x9 * 4, &fifoctrl0_rxfifoctrl2_flds},
    {"rxfifoctrl3", NULL, 0xa * 4, &fifoctrl0_rxfifoctrl3_flds},
    {"txfifoctrl0", NULL, 0xf * 4, &fifoctrl0_txfifoctrl0_flds},
    {"txfifoctrl1", NULL, 0x10 * 4, &fifoctrl0_txfifoctrl1_flds},
    {"txfifoctrl2", NULL, 0x11 * 4, &fifoctrl0_txfifoctrl2_flds},
    {"txfifoctrl3", NULL, 0x12 * 4, &fifoctrl0_txfifoctrl3_flds},
    {"chmap0", NULL, 0x17 * 4, &fifoctrl0_chmap0_flds},
    {"chmap1", NULL, 0x18 * 4, &fifoctrl0_chmap1_flds},
    {"parityctrl", NULL, 0x30 * 4, &fifoctrl0_parityctrl_flds},
    {"fint0", NULL, 0x70 * 4, &fifoctrl0_fint0_flds},
    {"fint2", NULL, 0x72 * 4, &fifoctrl0_fint2_flds},
    {"spare1", NULL, 0xfe * 4, &fifoctrl0_spare1_flds},
    {"spare0", NULL, 0xff * 4, &fifoctrl0_spare0_flds},
};
cmd_arg_t fifoctrl0_list = {20, fifoctrl0_regs};

cmd_arg_item_t hsmcpcs0_regs[] = {
    {"ctrl1", NULL, 0x0 * 4, &hsmcpcs0_ctrl1_flds},
    {"status1", NULL, 0x1 * 4, &hsmcpcs0_status1_flds},
    {"mode", NULL, 0x10 * 4, &hsmcpcs0_mode_flds},
    {"mlgconfig", NULL, 0x11 * 4, &hsmcpcs0_mlgconfig_flds},
    {"sts1", NULL, 0x20 * 4, &hsmcpcs0_sts1_flds},
    {"sts2", NULL, 0x21 * 4, &hsmcpcs0_sts2_flds},
    {"testpatterna1", NULL, 0x22 * 4, &hsmcpcs0_testpatterna1_flds},
    {"testpatterna2", NULL, 0x23 * 4, &hsmcpcs0_testpatterna2_flds},
    {"testpatterna3", NULL, 0x24 * 4, &hsmcpcs0_testpatterna3_flds},
    {"testpatterna4", NULL, 0x25 * 4, &hsmcpcs0_testpatterna4_flds},
    {"testpatternb1", NULL, 0x26 * 4, &hsmcpcs0_testpatternb1_flds},
    {"testpatternb2", NULL, 0x27 * 4, &hsmcpcs0_testpatternb2_flds},
    {"testpatternb3", NULL, 0x28 * 4, &hsmcpcs0_testpatternb3_flds},
    {"testpatternb4", NULL, 0x29 * 4, &hsmcpcs0_testpatternb4_flds},
    {"testpatctrl", NULL, 0x2a * 4, &hsmcpcs0_testpatctrl_flds},
    {"testpatternerrors", NULL, 0x2b * 4, &hsmcpcs0_testpatternerrors_flds},
    {"bercounthi", NULL, 0x2c * 4, &hsmcpcs0_bercounthi_flds},
    {"erroredblockshi", NULL, 0x2d * 4, &hsmcpcs0_erroredblockshi_flds},
    {"counters0", NULL, 0x2e * 4, &hsmcpcs0_counters0_flds},
    {"counters1", NULL, 0x2f * 4, &hsmcpcs0_counters1_flds},
    {"counters2", NULL, 0x30 * 4, &hsmcpcs0_counters2_flds},
    {"algnstat1", NULL, 0x32 * 4, &hsmcpcs0_algnstat1_flds},
    {"algnstat2", NULL, 0x33 * 4, &hsmcpcs0_algnstat2_flds},
    {"algnstat3", NULL, 0x34 * 4, &hsmcpcs0_algnstat3_flds},
    {"algnstat4", NULL, 0x35 * 4, &hsmcpcs0_algnstat4_flds},
    {"amctrl", NULL, 0x50 * 4, &hsmcpcs0_amctrl_flds},
    {"am0low", NULL, 0x5c * 4, &hsmcpcs0_am0low_flds},
    {"am0hi", NULL, 0x5d * 4, &hsmcpcs0_am0hi_flds},
    {"am1low", NULL, 0x5e * 4, &hsmcpcs0_am1low_flds},
    {"am1hi", NULL, 0x5f * 4, &hsmcpcs0_am1hi_flds},
    {"am2low", NULL, 0x60 * 4, &hsmcpcs0_am2low_flds},
    {"am2hi", NULL, 0x61 * 4, &hsmcpcs0_am2hi_flds},
    {"am3low", NULL, 0x62 * 4, &hsmcpcs0_am3low_flds},
    {"am3hi", NULL, 0x63 * 4, &hsmcpcs0_am3hi_flds},
    {"fint0", NULL, 0x70 * 4, &hsmcpcs0_fint0_flds},
    {"lanemapping0", NULL, 0x90 * 4, &hsmcpcs0_lanemapping0_flds},
    {"lanemapping1", NULL, 0x91 * 4, &hsmcpcs0_lanemapping1_flds},
    {"lanemapping2", NULL, 0x92 * 4, &hsmcpcs0_lanemapping2_flds},
    {"lanemapping3", NULL, 0x93 * 4, &hsmcpcs0_lanemapping3_flds},
    {"lanemapping4", NULL, 0x94 * 4, &hsmcpcs0_lanemapping4_flds},
    {"lanemapping5", NULL, 0x95 * 4, &hsmcpcs0_lanemapping5_flds},
    {"lanemapping6", NULL, 0x96 * 4, &hsmcpcs0_lanemapping6_flds},
    {"lanemapping7", NULL, 0x97 * 4, &hsmcpcs0_lanemapping7_flds},
    {"lanemapping8", NULL, 0x98 * 4, &hsmcpcs0_lanemapping8_flds},
    {"lanemapping9", NULL, 0x99 * 4, &hsmcpcs0_lanemapping9_flds},
    {"lanemapping10", NULL, 0x9a * 4, &hsmcpcs0_lanemapping10_flds},
    {"lanemapping11", NULL, 0x9b * 4, &hsmcpcs0_lanemapping11_flds},
    {"lanemapping12", NULL, 0x9c * 4, &hsmcpcs0_lanemapping12_flds},
    {"lanemapping13", NULL, 0x9d * 4, &hsmcpcs0_lanemapping13_flds},
    {"lanemapping14", NULL, 0x9e * 4, &hsmcpcs0_lanemapping14_flds},
    {"lanemapping15", NULL, 0x9f * 4, &hsmcpcs0_lanemapping15_flds},
    {"lanemapping16", NULL, 0xa0 * 4, &hsmcpcs0_lanemapping16_flds},
    {"lanemapping17", NULL, 0xa1 * 4, &hsmcpcs0_lanemapping17_flds},
    {"lanemapping18", NULL, 0xa2 * 4, &hsmcpcs0_lanemapping18_flds},
    {"lanemapping19", NULL, 0xa3 * 4, &hsmcpcs0_lanemapping19_flds},
    {"biperrorsln0", NULL, 0xc8 * 4, &hsmcpcs0_biperrorsln0_flds},
    {"biperrorsln1", NULL, 0xc9 * 4, &hsmcpcs0_biperrorsln1_flds},
    {"biperrorsln2", NULL, 0xca * 4, &hsmcpcs0_biperrorsln2_flds},
    {"biperrorsln3", NULL, 0xcb * 4, &hsmcpcs0_biperrorsln3_flds},
    {"biperrorsln4", NULL, 0xcc * 4, &hsmcpcs0_biperrorsln4_flds},
    {"biperrorsln5", NULL, 0xcd * 4, &hsmcpcs0_biperrorsln5_flds},
    {"biperrorsln6", NULL, 0xce * 4, &hsmcpcs0_biperrorsln6_flds},
    {"biperrorsln7", NULL, 0xcf * 4, &hsmcpcs0_biperrorsln7_flds},
    {"biperrorsln8", NULL, 0xd0 * 4, &hsmcpcs0_biperrorsln8_flds},
    {"biperrorsln9", NULL, 0xd1 * 4, &hsmcpcs0_biperrorsln9_flds},
    {"biperrorsln10", NULL, 0xd2 * 4, &hsmcpcs0_biperrorsln10_flds},
    {"biperrorsln11", NULL, 0xd3 * 4, &hsmcpcs0_biperrorsln11_flds},
    {"biperrorsln12", NULL, 0xd4 * 4, &hsmcpcs0_biperrorsln12_flds},
    {"biperrorsln13", NULL, 0xd5 * 4, &hsmcpcs0_biperrorsln13_flds},
    {"biperrorsln14", NULL, 0xd6 * 4, &hsmcpcs0_biperrorsln14_flds},
    {"biperrorsln15", NULL, 0xd7 * 4, &hsmcpcs0_biperrorsln15_flds},
    {"biperrorsln16", NULL, 0xd8 * 4, &hsmcpcs0_biperrorsln16_flds},
    {"biperrorsln17", NULL, 0xd9 * 4, &hsmcpcs0_biperrorsln17_flds},
    {"biperrorsln18", NULL, 0xda * 4, &hsmcpcs0_biperrorsln18_flds},
    {"biperrorsln19", NULL, 0xdb * 4, &hsmcpcs0_biperrorsln19_flds},
    {"dbgbiterrorgap", NULL, 0xf1 * 4, &hsmcpcs0_dbgbiterrorgap_flds},
    {"dbgsyncheadgap", NULL, 0xf2 * 4, &hsmcpcs0_dbgsyncheadgap_flds},
    {"dbgcodegrp0", NULL, 0xf3 * 4, &hsmcpcs0_dbgcodegrp0_flds},
    {"dbgcodegrp1", NULL, 0xf4 * 4, &hsmcpcs0_dbgcodegrp1_flds},
    {"dbgcodegrp2", NULL, 0xf5 * 4, &hsmcpcs0_dbgcodegrp2_flds},
    {"dbgcodegrp3", NULL, 0xf6 * 4, &hsmcpcs0_dbgcodegrp3_flds},
    {"dbgcodegrp4", NULL, 0xf7 * 4, &hsmcpcs0_dbgcodegrp4_flds},
    {"dbgctrl2", NULL, 0xf8 * 4, &hsmcpcs0_dbgctrl2_flds},
    {"dbgbersmovrd", NULL, 0xfb * 4, &hsmcpcs0_dbgbersmovrd_flds},
    {"spare0", NULL, 0xfc * 4, &hsmcpcs0_spare0_flds},
    {"dbgstat2", NULL, 0xfd * 4, &hsmcpcs0_dbgstat2_flds},
    {"dbgstat1", NULL, 0xfe * 4, &hsmcpcs0_dbgstat1_flds},
    {"dbgctrl1", NULL, 0xff * 4, &hsmcpcs0_dbgctrl1_flds},
};
cmd_arg_t hsmcpcs0_list = {88, hsmcpcs0_regs};

cmd_arg_item_t hsmcpcs1_regs[] = {
    {"ctrl1", NULL, 0x0 * 4, &hsmcpcs1_ctrl1_flds},
    {"status1", NULL, 0x1 * 4, &hsmcpcs1_status1_flds},
    {"mode", NULL, 0x10 * 4, &hsmcpcs1_mode_flds},
    {"sts1", NULL, 0x20 * 4, &hsmcpcs1_sts1_flds},
    {"sts2", NULL, 0x21 * 4, &hsmcpcs1_sts2_flds},
    {"testpatterna1", NULL, 0x22 * 4, &hsmcpcs1_testpatterna1_flds},
    {"testpatterna2", NULL, 0x23 * 4, &hsmcpcs1_testpatterna2_flds},
    {"testpatterna3", NULL, 0x24 * 4, &hsmcpcs1_testpatterna3_flds},
    {"testpatterna4", NULL, 0x25 * 4, &hsmcpcs1_testpatterna4_flds},
    {"testpatternb1", NULL, 0x26 * 4, &hsmcpcs1_testpatternb1_flds},
    {"testpatternb2", NULL, 0x27 * 4, &hsmcpcs1_testpatternb2_flds},
    {"testpatternb3", NULL, 0x28 * 4, &hsmcpcs1_testpatternb3_flds},
    {"testpatternb4", NULL, 0x29 * 4, &hsmcpcs1_testpatternb4_flds},
    {"testpatctrl", NULL, 0x2a * 4, &hsmcpcs1_testpatctrl_flds},
    {"testpatternerrors", NULL, 0x2b * 4, &hsmcpcs1_testpatternerrors_flds},
    {"bercounthi", NULL, 0x2c * 4, &hsmcpcs1_bercounthi_flds},
    {"erroredblockshi", NULL, 0x2d * 4, &hsmcpcs1_erroredblockshi_flds},
    {"counters0", NULL, 0x2e * 4, &hsmcpcs1_counters0_flds},
    {"counters1", NULL, 0x2f * 4, &hsmcpcs1_counters1_flds},
    {"counters2", NULL, 0x30 * 4, &hsmcpcs1_counters2_flds},
    {"algnstat1", NULL, 0x32 * 4, &hsmcpcs1_algnstat1_flds},
    {"amctrl", NULL, 0x50 * 4, &hsmcpcs1_amctrl_flds},
    {"fint0", NULL, 0x70 * 4, &hsmcpcs1_fint0_flds},
    {"dbgctrl2", NULL, 0xf8 * 4, &hsmcpcs1_dbgctrl2_flds},
    {"dbgbersmovrd", NULL, 0xfb * 4, &hsmcpcs1_dbgbersmovrd_flds},
    {"dbgctrl1", NULL, 0xff * 4, &hsmcpcs1_dbgctrl1_flds},
};
cmd_arg_t hsmcpcs1_list = {26, hsmcpcs1_regs};

cmd_arg_item_t hsmcpcs2_regs[] = {
    {"ctrl1", NULL, 0x0 * 4, &hsmcpcs2_ctrl1_flds},
    {"status1", NULL, 0x1 * 4, &hsmcpcs2_status1_flds},
    {"mode", NULL, 0x10 * 4, &hsmcpcs2_mode_flds},
    {"sts1", NULL, 0x20 * 4, &hsmcpcs2_sts1_flds},
    {"sts2", NULL, 0x21 * 4, &hsmcpcs2_sts2_flds},
    {"testpatterna1", NULL, 0x22 * 4, &hsmcpcs2_testpatterna1_flds},
    {"testpatterna2", NULL, 0x23 * 4, &hsmcpcs2_testpatterna2_flds},
    {"testpatterna3", NULL, 0x24 * 4, &hsmcpcs2_testpatterna3_flds},
    {"testpatterna4", NULL, 0x25 * 4, &hsmcpcs2_testpatterna4_flds},
    {"testpatternb1", NULL, 0x26 * 4, &hsmcpcs2_testpatternb1_flds},
    {"testpatternb2", NULL, 0x27 * 4, &hsmcpcs2_testpatternb2_flds},
    {"testpatternb3", NULL, 0x28 * 4, &hsmcpcs2_testpatternb3_flds},
    {"testpatternb4", NULL, 0x29 * 4, &hsmcpcs2_testpatternb4_flds},
    {"testpatctrl", NULL, 0x2a * 4, &hsmcpcs2_testpatctrl_flds},
    {"testpatternerrors", NULL, 0x2b * 4, &hsmcpcs2_testpatternerrors_flds},
    {"bercounthi", NULL, 0x2c * 4, &hsmcpcs2_bercounthi_flds},
    {"erroredblockshi", NULL, 0x2d * 4, &hsmcpcs2_erroredblockshi_flds},
    {"counters0", NULL, 0x2e * 4, &hsmcpcs2_counters0_flds},
    {"counters1", NULL, 0x2f * 4, &hsmcpcs2_counters1_flds},
    {"counters2", NULL, 0x30 * 4, &hsmcpcs2_counters2_flds},
    {"algnstat1", NULL, 0x32 * 4, &hsmcpcs2_algnstat1_flds},
    {"algnstat3", NULL, 0x34 * 4, &hsmcpcs2_algnstat3_flds},
    {"amctrl", NULL, 0x50 * 4, &hsmcpcs2_amctrl_flds},
    {"fint0", NULL, 0x70 * 4, &hsmcpcs2_fint0_flds},
    {"lanemapping0", NULL, 0x90 * 4, &hsmcpcs2_lanemapping0_flds},
    {"lanemapping1", NULL, 0x91 * 4, &hsmcpcs2_lanemapping1_flds},
    {"lanemapping2", NULL, 0x92 * 4, &hsmcpcs2_lanemapping2_flds},
    {"lanemapping3", NULL, 0x93 * 4, &hsmcpcs2_lanemapping3_flds},
    {"biperrorsln0", NULL, 0xc8 * 4, &hsmcpcs2_biperrorsln0_flds},
    {"biperrorsln1", NULL, 0xc9 * 4, &hsmcpcs2_biperrorsln1_flds},
    {"biperrorsln2", NULL, 0xca * 4, &hsmcpcs2_biperrorsln2_flds},
    {"biperrorsln3", NULL, 0xcb * 4, &hsmcpcs2_biperrorsln3_flds},
    {"dbgctrl2", NULL, 0xf8 * 4, &hsmcpcs2_dbgctrl2_flds},
    {"dbgbersmovrd", NULL, 0xfb * 4, &hsmcpcs2_dbgbersmovrd_flds},
    {"dbgctrl1", NULL, 0xff * 4, &hsmcpcs2_dbgctrl1_flds},
};
cmd_arg_t hsmcpcs2_list = {35, hsmcpcs2_regs};

cmd_arg_item_t hsmcpcs3_regs[] = {
    {"ctrl1", NULL, 0x0 * 4, &hsmcpcs3_ctrl1_flds},
    {"status1", NULL, 0x1 * 4, &hsmcpcs3_status1_flds},
    {"mode", NULL, 0x10 * 4, &hsmcpcs3_mode_flds},
    {"sts1", NULL, 0x20 * 4, &hsmcpcs3_sts1_flds},
    {"sts2", NULL, 0x21 * 4, &hsmcpcs3_sts2_flds},
    {"testpatterna1", NULL, 0x22 * 4, &hsmcpcs3_testpatterna1_flds},
    {"testpatterna2", NULL, 0x23 * 4, &hsmcpcs3_testpatterna2_flds},
    {"testpatterna3", NULL, 0x24 * 4, &hsmcpcs3_testpatterna3_flds},
    {"testpatterna4", NULL, 0x25 * 4, &hsmcpcs3_testpatterna4_flds},
    {"testpatternb1", NULL, 0x26 * 4, &hsmcpcs3_testpatternb1_flds},
    {"testpatternb2", NULL, 0x27 * 4, &hsmcpcs3_testpatternb2_flds},
    {"testpatternb3", NULL, 0x28 * 4, &hsmcpcs3_testpatternb3_flds},
    {"testpatternb4", NULL, 0x29 * 4, &hsmcpcs3_testpatternb4_flds},
    {"testpatctrl", NULL, 0x2a * 4, &hsmcpcs3_testpatctrl_flds},
    {"testpatternerrors", NULL, 0x2b * 4, &hsmcpcs3_testpatternerrors_flds},
    {"bercounthi", NULL, 0x2c * 4, &hsmcpcs3_bercounthi_flds},
    {"erroredblockshi", NULL, 0x2d * 4, &hsmcpcs3_erroredblockshi_flds},
    {"counters0", NULL, 0x2e * 4, &hsmcpcs3_counters0_flds},
    {"counters1", NULL, 0x2f * 4, &hsmcpcs3_counters1_flds},
    {"counters2", NULL, 0x30 * 4, &hsmcpcs3_counters2_flds},
    {"algnstat1", NULL, 0x32 * 4, &hsmcpcs3_algnstat1_flds},
    {"amctrl", NULL, 0x50 * 4, &hsmcpcs3_amctrl_flds},
    {"fint0", NULL, 0x70 * 4, &hsmcpcs3_fint0_flds},
    {"dbgctrl2", NULL, 0xf8 * 4, &hsmcpcs3_dbgctrl2_flds},
    {"dbgbersmovrd", NULL, 0xfb * 4, &hsmcpcs3_dbgbersmovrd_flds},
    {"dbgctrl1", NULL, 0xff * 4, &hsmcpcs3_dbgctrl1_flds},
};
cmd_arg_t hsmcpcs3_list = {26, hsmcpcs3_regs};

cmd_arg_item_t fecrs0_regs[] = {
    {"ctrl", NULL, 0xc8 * 4, &fecrs0_ctrl_flds},
    {"sts", NULL, 0xc9 * 4, &fecrs0_sts_flds},
    {"corrcntlo", NULL, 0xca * 4, &fecrs0_corrcntlo_flds},
    {"corrcnthi", NULL, 0xcb * 4, &fecrs0_corrcnthi_flds},
    {"uncorrcntlo", NULL, 0xcc * 4, &fecrs0_uncorrcntlo_flds},
    {"uncorrcnthi", NULL, 0xcd * 4, &fecrs0_uncorrcnthi_flds},
    {"lanemapping", NULL, 0xce * 4, &fecrs0_lanemapping_flds},
    {"serlane0lo", NULL, 0xd2 * 4, &fecrs0_serlane0lo_flds},
    {"serlane0hi", NULL, 0xd3 * 4, &fecrs0_serlane0hi_flds},
    {"serlane1lo", NULL, 0xd4 * 4, &fecrs0_serlane1lo_flds},
    {"serlane1hi", NULL, 0xd5 * 4, &fecrs0_serlane1hi_flds},
    {"serlane2lo", NULL, 0xd6 * 4, &fecrs0_serlane2lo_flds},
    {"serlane2hi", NULL, 0xd7 * 4, &fecrs0_serlane2hi_flds},
    {"serlane3lo", NULL, 0xd8 * 4, &fecrs0_serlane3lo_flds},
    {"serlane3hi", NULL, 0xd9 * 4, &fecrs0_serlane3hi_flds},
    {"dbgctrl", NULL, 0x0 * 4, &fecrs0_dbgctrl_flds},
    {"dbgsts", NULL, 0x1 * 4, &fecrs0_dbgsts_flds},
    {"dbgberintthres", NULL, 0x5 * 4, &fecrs0_dbgberintthres_flds},
    {"frcinvalidtimerlo", NULL, 0x6 * 4, &fecrs0_frcinvalidtimerlo_flds},
    {"frcinvalidtimerhi", NULL, 0x7 * 4, &fecrs0_frcinvalidtimerhi_flds},
    {"fint0", NULL, 0x70 * 4, &fecrs0_fint0_flds},
    {"spare0", NULL, 0xfc * 4, &fecrs0_spare0_flds},
    {"ieeecfg", NULL, 0xfd * 4, &fecrs0_ieeecfg_flds},
};
cmd_arg_t fecrs0_list = {23, fecrs0_regs};

cmd_arg_item_t fecrs1_regs[] = {
    {"ctrl", NULL, 0xc8 * 4, &fecrs1_ctrl_flds},
    {"sts", NULL, 0xc9 * 4, &fecrs1_sts_flds},
    {"corrcntlo", NULL, 0xca * 4, &fecrs1_corrcntlo_flds},
    {"corrcnthi", NULL, 0xcb * 4, &fecrs1_corrcnthi_flds},
    {"uncorrcntlo", NULL, 0xcc * 4, &fecrs1_uncorrcntlo_flds},
    {"uncorrcnthi", NULL, 0xcd * 4, &fecrs1_uncorrcnthi_flds},
    {"lanemapping", NULL, 0xce * 4, &fecrs1_lanemapping_flds},
    {"serlane0lo", NULL, 0xd2 * 4, &fecrs1_serlane0lo_flds},
    {"serlane0hi", NULL, 0xd3 * 4, &fecrs1_serlane0hi_flds},
    {"serlane1lo", NULL, 0xd4 * 4, &fecrs1_serlane1lo_flds},
    {"serlane1hi", NULL, 0xd5 * 4, &fecrs1_serlane1hi_flds},
    {"serlane2lo", NULL, 0xd6 * 4, &fecrs1_serlane2lo_flds},
    {"serlane2hi", NULL, 0xd7 * 4, &fecrs1_serlane2hi_flds},
    {"serlane3lo", NULL, 0xd8 * 4, &fecrs1_serlane3lo_flds},
    {"serlane3hi", NULL, 0xd9 * 4, &fecrs1_serlane3hi_flds},
    {"dbgctrl", NULL, 0x0 * 4, &fecrs1_dbgctrl_flds},
    {"dbgsts", NULL, 0x1 * 4, &fecrs1_dbgsts_flds},
    {"dbgberintthres", NULL, 0x5 * 4, &fecrs1_dbgberintthres_flds},
    {"frcinvalidtimerlo", NULL, 0x6 * 4, &fecrs1_frcinvalidtimerlo_flds},
    {"frcinvalidtimerhi", NULL, 0x7 * 4, &fecrs1_frcinvalidtimerhi_flds},
    {"fint0", NULL, 0x70 * 4, &fecrs1_fint0_flds},
    {"spare0", NULL, 0xfc * 4, &fecrs1_spare0_flds},
    {"ieeecfg", NULL, 0xfd * 4, &fecrs1_ieeecfg_flds},
};
cmd_arg_t fecrs1_list = {23, fecrs1_regs};

cmd_arg_item_t fecrs2_regs[] = {
    {"ctrl", NULL, 0xc8 * 4, &fecrs2_ctrl_flds},
    {"sts", NULL, 0xc9 * 4, &fecrs2_sts_flds},
    {"corrcntlo", NULL, 0xca * 4, &fecrs2_corrcntlo_flds},
    {"corrcnthi", NULL, 0xcb * 4, &fecrs2_corrcnthi_flds},
    {"uncorrcntlo", NULL, 0xcc * 4, &fecrs2_uncorrcntlo_flds},
    {"uncorrcnthi", NULL, 0xcd * 4, &fecrs2_uncorrcnthi_flds},
    {"lanemapping", NULL, 0xce * 4, &fecrs2_lanemapping_flds},
    {"serlane0lo", NULL, 0xd2 * 4, &fecrs2_serlane0lo_flds},
    {"serlane0hi", NULL, 0xd3 * 4, &fecrs2_serlane0hi_flds},
    {"serlane1lo", NULL, 0xd4 * 4, &fecrs2_serlane1lo_flds},
    {"serlane1hi", NULL, 0xd5 * 4, &fecrs2_serlane1hi_flds},
    {"serlane2lo", NULL, 0xd6 * 4, &fecrs2_serlane2lo_flds},
    {"serlane2hi", NULL, 0xd7 * 4, &fecrs2_serlane2hi_flds},
    {"serlane3lo", NULL, 0xd8 * 4, &fecrs2_serlane3lo_flds},
    {"serlane3hi", NULL, 0xd9 * 4, &fecrs2_serlane3hi_flds},
    {"dbgctrl", NULL, 0x0 * 4, &fecrs2_dbgctrl_flds},
    {"dbgsts", NULL, 0x1 * 4, &fecrs2_dbgsts_flds},
    {"dbgberintthres", NULL, 0x5 * 4, &fecrs2_dbgberintthres_flds},
    {"frcinvalidtimerlo", NULL, 0x6 * 4, &fecrs2_frcinvalidtimerlo_flds},
    {"frcinvalidtimerhi", NULL, 0x7 * 4, &fecrs2_frcinvalidtimerhi_flds},
    {"fint0", NULL, 0x70 * 4, &fecrs2_fint0_flds},
    {"spare0", NULL, 0xfc * 4, &fecrs2_spare0_flds},
    {"ieeecfg", NULL, 0xfd * 4, &fecrs2_ieeecfg_flds},
};
cmd_arg_t fecrs2_list = {23, fecrs2_regs};

cmd_arg_item_t fecrs3_regs[] = {
    {"ctrl", NULL, 0xc8 * 4, &fecrs3_ctrl_flds},
    {"sts", NULL, 0xc9 * 4, &fecrs3_sts_flds},
    {"corrcntlo", NULL, 0xca * 4, &fecrs3_corrcntlo_flds},
    {"corrcnthi", NULL, 0xcb * 4, &fecrs3_corrcnthi_flds},
    {"uncorrcntlo", NULL, 0xcc * 4, &fecrs3_uncorrcntlo_flds},
    {"uncorrcnthi", NULL, 0xcd * 4, &fecrs3_uncorrcnthi_flds},
    {"lanemapping", NULL, 0xce * 4, &fecrs3_lanemapping_flds},
    {"serlane0lo", NULL, 0xd2 * 4, &fecrs3_serlane0lo_flds},
    {"serlane0hi", NULL, 0xd3 * 4, &fecrs3_serlane0hi_flds},
    {"serlane1lo", NULL, 0xd4 * 4, &fecrs3_serlane1lo_flds},
    {"serlane1hi", NULL, 0xd5 * 4, &fecrs3_serlane1hi_flds},
    {"serlane2lo", NULL, 0xd6 * 4, &fecrs3_serlane2lo_flds},
    {"serlane2hi", NULL, 0xd7 * 4, &fecrs3_serlane2hi_flds},
    {"serlane3lo", NULL, 0xd8 * 4, &fecrs3_serlane3lo_flds},
    {"serlane3hi", NULL, 0xd9 * 4, &fecrs3_serlane3hi_flds},
    {"dbgctrl", NULL, 0x0 * 4, &fecrs3_dbgctrl_flds},
    {"dbgsts", NULL, 0x1 * 4, &fecrs3_dbgsts_flds},
    {"dbgberintthres", NULL, 0x5 * 4, &fecrs3_dbgberintthres_flds},
    {"frcinvalidtimerlo", NULL, 0x6 * 4, &fecrs3_frcinvalidtimerlo_flds},
    {"frcinvalidtimerhi", NULL, 0x7 * 4, &fecrs3_frcinvalidtimerhi_flds},
    {"fint0", NULL, 0x70 * 4, &fecrs3_fint0_flds},
    {"spare0", NULL, 0xfc * 4, &fecrs3_spare0_flds},
    {"ieeecfg", NULL, 0xfd * 4, &fecrs3_ieeecfg_flds},
};
cmd_arg_t fecrs3_list = {23, fecrs3_regs};

cmd_arg_item_t fecfc0_regs[] = {
    {"fint0", NULL, 0x80 * 4, &fecfc0_fint0_flds},
    {"sts", NULL, 0xaa * 4, &fecfc0_sts_flds},
    {"cfg", NULL, 0xab * 4, &fecfc0_cfg_flds},
    {"corrblockscounterhi", NULL, 0xac * 4, &fecfc0_corrblockscounterhi_flds},
    {"corrblockscounterlo", NULL, 0xad * 4, &fecfc0_corrblockscounterlo_flds},
    {"uncorrblockscounterhi",
     NULL,
     0xae * 4,
     &fecfc0_uncorrblockscounterhi_flds},
    {"uncorrblockscounterlo",
     NULL,
     0xaf * 4,
     &fecfc0_uncorrblockscounterlo_flds},
    {"stsvl1", NULL, 0xba * 4, &fecfc0_stsvl1_flds},
    {"corrblockscounterhivl1",
     NULL,
     0xbc * 4,
     &fecfc0_corrblockscounterhivl1_flds},
    {"corrblockscounterlovl1",
     NULL,
     0xbd * 4,
     &fecfc0_corrblockscounterlovl1_flds},
    {"uncorrblockscounterhivl1",
     NULL,
     0xbe * 4,
     &fecfc0_uncorrblockscounterhivl1_flds},
    {"uncorrblockscounterlovl1",
     NULL,
     0xbf * 4,
     &fecfc0_uncorrblockscounterlovl1_flds},
    {"dbgvl1", NULL, 0xd0 * 4, &fecfc0_dbgvl1_flds},
    {"dbgcorrbitscounterhivl1",
     NULL,
     0xd1 * 4,
     &fecfc0_dbgcorrbitscounterhivl1_flds},
    {"dbgcorrbitscounterlovl1",
     NULL,
     0xd2 * 4,
     &fecfc0_dbgcorrbitscounterlovl1_flds},
    {"dbgblockcounthivl1", NULL, 0xd3 * 4, &fecfc0_dbgblockcounthivl1_flds},
    {"dbgblockcountlovl1", NULL, 0xd4 * 4, &fecfc0_dbgblockcountlovl1_flds},
    {"dbg", NULL, 0xe0 * 4, &fecfc0_dbg_flds},
    {"dbgcorrbitscounterhi", NULL, 0xe1 * 4, &fecfc0_dbgcorrbitscounterhi_flds},
    {"dbgcorrbitscounterlo", NULL, 0xe2 * 4, &fecfc0_dbgcorrbitscounterlo_flds},
    {"dbgblockcounthi", NULL, 0xe3 * 4, &fecfc0_dbgblockcounthi_flds},
    {"dbgblockcountlo", NULL, 0xe4 * 4, &fecfc0_dbgblockcountlo_flds},
    {"spare0", NULL, 0xff * 4, &fecfc0_spare0_flds},
};
cmd_arg_t fecfc0_list = {23, fecfc0_regs};

cmd_arg_item_t fecfc1_regs[] = {
    {"fint0", NULL, 0x80 * 4, &fecfc1_fint0_flds},
    {"sts", NULL, 0xaa * 4, &fecfc1_sts_flds},
    {"cfg", NULL, 0xab * 4, &fecfc1_cfg_flds},
    {"corrblockscounterhi", NULL, 0xac * 4, &fecfc1_corrblockscounterhi_flds},
    {"corrblockscounterlo", NULL, 0xad * 4, &fecfc1_corrblockscounterlo_flds},
    {"uncorrblockscounterhi",
     NULL,
     0xae * 4,
     &fecfc1_uncorrblockscounterhi_flds},
    {"uncorrblockscounterlo",
     NULL,
     0xaf * 4,
     &fecfc1_uncorrblockscounterlo_flds},
    {"stsvl1", NULL, 0xba * 4, &fecfc1_stsvl1_flds},
    {"corrblockscounterhivl1",
     NULL,
     0xbc * 4,
     &fecfc1_corrblockscounterhivl1_flds},
    {"corrblockscounterlovl1",
     NULL,
     0xbd * 4,
     &fecfc1_corrblockscounterlovl1_flds},
    {"uncorrblockscounterhivl1",
     NULL,
     0xbe * 4,
     &fecfc1_uncorrblockscounterhivl1_flds},
    {"uncorrblockscounterlovl1",
     NULL,
     0xbf * 4,
     &fecfc1_uncorrblockscounterlovl1_flds},
    {"dbgvl1", NULL, 0xd0 * 4, &fecfc1_dbgvl1_flds},
    {"dbgcorrbitscounterhivl1",
     NULL,
     0xd1 * 4,
     &fecfc1_dbgcorrbitscounterhivl1_flds},
    {"dbgcorrbitscounterlovl1",
     NULL,
     0xd2 * 4,
     &fecfc1_dbgcorrbitscounterlovl1_flds},
    {"dbgblockcounthivl1", NULL, 0xd3 * 4, &fecfc1_dbgblockcounthivl1_flds},
    {"dbgblockcountlovl1", NULL, 0xd4 * 4, &fecfc1_dbgblockcountlovl1_flds},
    {"dbg", NULL, 0xe0 * 4, &fecfc1_dbg_flds},
    {"dbgcorrbitscounterhi", NULL, 0xe1 * 4, &fecfc1_dbgcorrbitscounterhi_flds},
    {"dbgcorrbitscounterlo", NULL, 0xe2 * 4, &fecfc1_dbgcorrbitscounterlo_flds},
    {"dbgblockcounthi", NULL, 0xe3 * 4, &fecfc1_dbgblockcounthi_flds},
    {"dbgblockcountlo", NULL, 0xe4 * 4, &fecfc1_dbgblockcountlo_flds},
};
cmd_arg_t fecfc1_list = {22, fecfc1_regs};

cmd_arg_item_t fecfc2_regs[] = {
    {"fint0", NULL, 0x80 * 4, &fecfc2_fint0_flds},
    {"sts", NULL, 0xaa * 4, &fecfc2_sts_flds},
    {"cfg", NULL, 0xab * 4, &fecfc2_cfg_flds},
    {"corrblockscounterhi", NULL, 0xac * 4, &fecfc2_corrblockscounterhi_flds},
    {"corrblockscounterlo", NULL, 0xad * 4, &fecfc2_corrblockscounterlo_flds},
    {"uncorrblockscounterhi",
     NULL,
     0xae * 4,
     &fecfc2_uncorrblockscounterhi_flds},
    {"uncorrblockscounterlo",
     NULL,
     0xaf * 4,
     &fecfc2_uncorrblockscounterlo_flds},
    {"stsvl1", NULL, 0xba * 4, &fecfc2_stsvl1_flds},
    {"corrblockscounterhivl1",
     NULL,
     0xbc * 4,
     &fecfc2_corrblockscounterhivl1_flds},
    {"corrblockscounterlovl1",
     NULL,
     0xbd * 4,
     &fecfc2_corrblockscounterlovl1_flds},
    {"uncorrblockscounterhivl1",
     NULL,
     0xbe * 4,
     &fecfc2_uncorrblockscounterhivl1_flds},
    {"uncorrblockscounterlovl1",
     NULL,
     0xbf * 4,
     &fecfc2_uncorrblockscounterlovl1_flds},
    {"dbgvl1", NULL, 0xd0 * 4, &fecfc2_dbgvl1_flds},
    {"dbgcorrbitscounterhivl1",
     NULL,
     0xd1 * 4,
     &fecfc2_dbgcorrbitscounterhivl1_flds},
    {"dbgcorrbitscounterlovl1",
     NULL,
     0xd2 * 4,
     &fecfc2_dbgcorrbitscounterlovl1_flds},
    {"dbgblockcounthivl1", NULL, 0xd3 * 4, &fecfc2_dbgblockcounthivl1_flds},
    {"dbgblockcountlovl1", NULL, 0xd4 * 4, &fecfc2_dbgblockcountlovl1_flds},
    {"dbg", NULL, 0xe0 * 4, &fecfc2_dbg_flds},
    {"dbgcorrbitscounterhi", NULL, 0xe1 * 4, &fecfc2_dbgcorrbitscounterhi_flds},
    {"dbgcorrbitscounterlo", NULL, 0xe2 * 4, &fecfc2_dbgcorrbitscounterlo_flds},
    {"dbgblockcounthi", NULL, 0xe3 * 4, &fecfc2_dbgblockcounthi_flds},
    {"dbgblockcountlo", NULL, 0xe4 * 4, &fecfc2_dbgblockcountlo_flds},
};
cmd_arg_t fecfc2_list = {22, fecfc2_regs};

cmd_arg_item_t fecfc3_regs[] = {
    {"fint0", NULL, 0x80 * 4, &fecfc3_fint0_flds},
    {"sts", NULL, 0xaa * 4, &fecfc3_sts_flds},
    {"cfg", NULL, 0xab * 4, &fecfc3_cfg_flds},
    {"corrblockscounterhi", NULL, 0xac * 4, &fecfc3_corrblockscounterhi_flds},
    {"corrblockscounterlo", NULL, 0xad * 4, &fecfc3_corrblockscounterlo_flds},
    {"uncorrblockscounterhi",
     NULL,
     0xae * 4,
     &fecfc3_uncorrblockscounterhi_flds},
    {"uncorrblockscounterlo",
     NULL,
     0xaf * 4,
     &fecfc3_uncorrblockscounterlo_flds},
    {"stsvl1", NULL, 0xba * 4, &fecfc3_stsvl1_flds},
    {"corrblockscounterhivl1",
     NULL,
     0xbc * 4,
     &fecfc3_corrblockscounterhivl1_flds},
    {"corrblockscounterlovl1",
     NULL,
     0xbd * 4,
     &fecfc3_corrblockscounterlovl1_flds},
    {"uncorrblockscounterhivl1",
     NULL,
     0xbe * 4,
     &fecfc3_uncorrblockscounterhivl1_flds},
    {"uncorrblockscounterlovl1",
     NULL,
     0xbf * 4,
     &fecfc3_uncorrblockscounterlovl1_flds},
    {"dbgvl1", NULL, 0xd0 * 4, &fecfc3_dbgvl1_flds},
    {"dbgcorrbitscounterhivl1",
     NULL,
     0xd1 * 4,
     &fecfc3_dbgcorrbitscounterhivl1_flds},
    {"dbgcorrbitscounterlovl1",
     NULL,
     0xd2 * 4,
     &fecfc3_dbgcorrbitscounterlovl1_flds},
    {"dbgblockcounthivl1", NULL, 0xd3 * 4, &fecfc3_dbgblockcounthivl1_flds},
    {"dbgblockcountlovl1", NULL, 0xd4 * 4, &fecfc3_dbgblockcountlovl1_flds},
    {"dbg", NULL, 0xe0 * 4, &fecfc3_dbg_flds},
    {"dbgcorrbitscounterhi", NULL, 0xe1 * 4, &fecfc3_dbgcorrbitscounterhi_flds},
    {"dbgcorrbitscounterlo", NULL, 0xe2 * 4, &fecfc3_dbgcorrbitscounterlo_flds},
    {"dbgblockcounthi", NULL, 0xe3 * 4, &fecfc3_dbgblockcounthi_flds},
    {"dbgblockcountlo", NULL, 0xe4 * 4, &fecfc3_dbgblockcountlo_flds},
};
cmd_arg_t fecfc3_list = {22, fecfc3_regs};

cmd_arg_item_t lsmcpcs0_regs[] = {
    {"ctrl", NULL, 0x0 * 4, &lsmcpcs0_ctrl_flds},
    {"mradvability", NULL, 0x1 * 4, &lsmcpcs0_mradvability_flds},
    {"rxconfig", NULL, 0x2 * 4, &lsmcpcs0_rxconfig_flds},
    {"lnktimer", NULL, 0x3 * 4, &lsmcpcs0_lnktimer_flds},
    {"lnktimer2", NULL, 0x13 * 4, &lsmcpcs0_lnktimer2_flds},
    {"disperrcnt", NULL, 0x4 * 4, &lsmcpcs0_disperrcnt_flds},
    {"invcodecnt", NULL, 0x5 * 4, &lsmcpcs0_invcodecnt_flds},
    {"sts", NULL, 0x6 * 4, &lsmcpcs0_sts_flds},
    {"fint0", NULL, 0x70 * 4, &lsmcpcs0_fint0_flds},
    {"spare1", NULL, 0xfe * 4, &lsmcpcs0_spare1_flds},
};
cmd_arg_t lsmcpcs0_list = {10, lsmcpcs0_regs};

cmd_arg_item_t lsmcpcs1_regs[] = {
    {"ctrl", NULL, 0x0 * 4, &lsmcpcs1_ctrl_flds},
    {"mradvability", NULL, 0x1 * 4, &lsmcpcs1_mradvability_flds},
    {"rxconfig", NULL, 0x2 * 4, &lsmcpcs1_rxconfig_flds},
    {"lnktimer", NULL, 0x3 * 4, &lsmcpcs1_lnktimer_flds},
    {"lnktimer2", NULL, 0x13 * 4, &lsmcpcs1_lnktimer2_flds},
    {"disperrcnt", NULL, 0x4 * 4, &lsmcpcs1_disperrcnt_flds},
    {"invcodecnt", NULL, 0x5 * 4, &lsmcpcs1_invcodecnt_flds},
    {"sts", NULL, 0x6 * 4, &lsmcpcs1_sts_flds},
    {"fint0", NULL, 0x70 * 4, &lsmcpcs1_fint0_flds},
};
cmd_arg_t lsmcpcs1_list = {9, lsmcpcs1_regs};

cmd_arg_item_t lsmcpcs2_regs[] = {
    {"ctrl", NULL, 0x0 * 4, &lsmcpcs2_ctrl_flds},
    {"mradvability", NULL, 0x1 * 4, &lsmcpcs2_mradvability_flds},
    {"rxconfig", NULL, 0x2 * 4, &lsmcpcs2_rxconfig_flds},
    {"lnktimer", NULL, 0x3 * 4, &lsmcpcs2_lnktimer_flds},
    {"lnktimer2", NULL, 0x13 * 4, &lsmcpcs2_lnktimer2_flds},
    {"disperrcnt", NULL, 0x4 * 4, &lsmcpcs2_disperrcnt_flds},
    {"invcodecnt", NULL, 0x5 * 4, &lsmcpcs2_invcodecnt_flds},
    {"sts", NULL, 0x6 * 4, &lsmcpcs2_sts_flds},
    {"fint0", NULL, 0x70 * 4, &lsmcpcs2_fint0_flds},
};
cmd_arg_t lsmcpcs2_list = {9, lsmcpcs2_regs};

cmd_arg_item_t lsmcpcs3_regs[] = {
    {"ctrl", NULL, 0x0 * 4, &lsmcpcs3_ctrl_flds},
    {"mradvability", NULL, 0x1 * 4, &lsmcpcs3_mradvability_flds},
    {"rxconfig", NULL, 0x2 * 4, &lsmcpcs3_rxconfig_flds},
    {"lnktimer", NULL, 0x3 * 4, &lsmcpcs3_lnktimer_flds},
    {"lnktimer2", NULL, 0x13 * 4, &lsmcpcs3_lnktimer2_flds},
    {"disperrcnt", NULL, 0x4 * 4, &lsmcpcs3_disperrcnt_flds},
    {"invcodecnt", NULL, 0x5 * 4, &lsmcpcs3_invcodecnt_flds},
    {"sts", NULL, 0x6 * 4, &lsmcpcs3_sts_flds},
    {"fint0", NULL, 0x70 * 4, &lsmcpcs3_fint0_flds},
};
cmd_arg_t lsmcpcs3_list = {9, lsmcpcs3_regs};

cmd_arg_item_t bpan0_regs[] = {
    {"ctrl1", NULL, 0x0 * 4, &bpan0_ctrl1_flds},
    {"status1", NULL, 0x1 * 4, &bpan0_status1_flds},
    {"tnonce", NULL, 0x2 * 4, &bpan0_tnonce_flds},
    {"enonce", NULL, 0x3 * 4, &bpan0_enonce_flds},
    {"abrctrl1", NULL, 0x4 * 4, &bpan0_abrctrl1_flds},
    {"abrctrl2", NULL, 0x5 * 4, &bpan0_abrctrl2_flds},
    {"pagetestmaxtimer", NULL, 0x6 * 4, &bpan0_pagetestmaxtimer_flds},
    {"pagetestmintimer", NULL, 0x7 * 4, &bpan0_pagetestmintimer_flds},
    {"txbasepagelo", NULL, 0x10 * 4, &bpan0_txbasepagelo_flds},
    {"txbasepagemid", NULL, 0x11 * 4, &bpan0_txbasepagemid_flds},
    {"txbasepagehi", NULL, 0x12 * 4, &bpan0_txbasepagehi_flds},
    {"rxbasepagelo", NULL, 0x13 * 4, &bpan0_rxbasepagelo_flds},
    {"rxbasepagemid", NULL, 0x14 * 4, &bpan0_rxbasepagemid_flds},
    {"rxbasepagehi", NULL, 0x15 * 4, &bpan0_rxbasepagehi_flds},
    {"txnextpagelo", NULL, 0x16 * 4, &bpan0_txnextpagelo_flds},
    {"txnextpagemid", NULL, 0x17 * 4, &bpan0_txnextpagemid_flds},
    {"txnextpagehi", NULL, 0x18 * 4, &bpan0_txnextpagehi_flds},
    {"rxnextpagelo", NULL, 0x19 * 4, &bpan0_rxnextpagelo_flds},
    {"rxnextpagemid", NULL, 0x1a * 4, &bpan0_rxnextpagemid_flds},
    {"rxnextpagehi", NULL, 0x1b * 4, &bpan0_rxnextpagehi_flds},
    {"fint0", NULL, 0x70 * 4, &bpan0_fint0_flds},
    {"intstatus", NULL, 0xf0 * 4, &bpan0_intstatus_flds},
};
cmd_arg_t bpan0_list = {22, bpan0_regs};

cmd_arg_item_t bpan1_regs[] = {
    {"ctrl1", NULL, 0x0 * 4, &bpan1_ctrl1_flds},
    {"status1", NULL, 0x1 * 4, &bpan1_status1_flds},
    {"tnonce", NULL, 0x2 * 4, &bpan1_tnonce_flds},
    {"enonce", NULL, 0x3 * 4, &bpan1_enonce_flds},
    {"abrctrl1", NULL, 0x4 * 4, &bpan1_abrctrl1_flds},
    {"abrctrl2", NULL, 0x5 * 4, &bpan1_abrctrl2_flds},
    {"pagetestmaxtimer", NULL, 0x6 * 4, &bpan1_pagetestmaxtimer_flds},
    {"pagetestmintimer", NULL, 0x7 * 4, &bpan1_pagetestmintimer_flds},
    {"txbasepagelo", NULL, 0x10 * 4, &bpan1_txbasepagelo_flds},
    {"txbasepagemid", NULL, 0x11 * 4, &bpan1_txbasepagemid_flds},
    {"txbasepagehi", NULL, 0x12 * 4, &bpan1_txbasepagehi_flds},
    {"rxbasepagelo", NULL, 0x13 * 4, &bpan1_rxbasepagelo_flds},
    {"rxbasepagemid", NULL, 0x14 * 4, &bpan1_rxbasepagemid_flds},
    {"rxbasepagehi", NULL, 0x15 * 4, &bpan1_rxbasepagehi_flds},
    {"txnextpagelo", NULL, 0x16 * 4, &bpan1_txnextpagelo_flds},
    {"txnextpagemid", NULL, 0x17 * 4, &bpan1_txnextpagemid_flds},
    {"txnextpagehi", NULL, 0x18 * 4, &bpan1_txnextpagehi_flds},
    {"rxnextpagelo", NULL, 0x19 * 4, &bpan1_rxnextpagelo_flds},
    {"rxnextpagemid", NULL, 0x1a * 4, &bpan1_rxnextpagemid_flds},
    {"rxnextpagehi", NULL, 0x1b * 4, &bpan1_rxnextpagehi_flds},
    {"fint0", NULL, 0x70 * 4, &bpan1_fint0_flds},
    {"intstatus", NULL, 0xf0 * 4, &bpan1_intstatus_flds},
};
cmd_arg_t bpan1_list = {22, bpan1_regs};

cmd_arg_item_t bpan2_regs[] = {
    {"ctrl1", NULL, 0x0 * 4, &bpan2_ctrl1_flds},
    {"status1", NULL, 0x1 * 4, &bpan2_status1_flds},
    {"tnonce", NULL, 0x2 * 4, &bpan2_tnonce_flds},
    {"enonce", NULL, 0x3 * 4, &bpan2_enonce_flds},
    {"abrctrl1", NULL, 0x4 * 4, &bpan2_abrctrl1_flds},
    {"abrctrl2", NULL, 0x5 * 4, &bpan2_abrctrl2_flds},
    {"pagetestmaxtimer", NULL, 0x6 * 4, &bpan2_pagetestmaxtimer_flds},
    {"pagetestmintimer", NULL, 0x7 * 4, &bpan2_pagetestmintimer_flds},
    {"txbasepagelo", NULL, 0x10 * 4, &bpan2_txbasepagelo_flds},
    {"txbasepagemid", NULL, 0x11 * 4, &bpan2_txbasepagemid_flds},
    {"txbasepagehi", NULL, 0x12 * 4, &bpan2_txbasepagehi_flds},
    {"rxbasepagelo", NULL, 0x13 * 4, &bpan2_rxbasepagelo_flds},
    {"rxbasepagemid", NULL, 0x14 * 4, &bpan2_rxbasepagemid_flds},
    {"rxbasepagehi", NULL, 0x15 * 4, &bpan2_rxbasepagehi_flds},
    {"txnextpagelo", NULL, 0x16 * 4, &bpan2_txnextpagelo_flds},
    {"txnextpagemid", NULL, 0x17 * 4, &bpan2_txnextpagemid_flds},
    {"txnextpagehi", NULL, 0x18 * 4, &bpan2_txnextpagehi_flds},
    {"rxnextpagelo", NULL, 0x19 * 4, &bpan2_rxnextpagelo_flds},
    {"rxnextpagemid", NULL, 0x1a * 4, &bpan2_rxnextpagemid_flds},
    {"rxnextpagehi", NULL, 0x1b * 4, &bpan2_rxnextpagehi_flds},
    {"fint0", NULL, 0x70 * 4, &bpan2_fint0_flds},
    {"intstatus", NULL, 0xf0 * 4, &bpan2_intstatus_flds},
};
cmd_arg_t bpan2_list = {22, bpan2_regs};

cmd_arg_item_t bpan3_regs[] = {
    {"ctrl1", NULL, 0x0 * 4, &bpan3_ctrl1_flds},
    {"status1", NULL, 0x1 * 4, &bpan3_status1_flds},
    {"tnonce", NULL, 0x2 * 4, &bpan3_tnonce_flds},
    {"enonce", NULL, 0x3 * 4, &bpan3_enonce_flds},
    {"abrctrl1", NULL, 0x4 * 4, &bpan3_abrctrl1_flds},
    {"abrctrl2", NULL, 0x5 * 4, &bpan3_abrctrl2_flds},
    {"pagetestmaxtimer", NULL, 0x6 * 4, &bpan3_pagetestmaxtimer_flds},
    {"pagetestmintimer", NULL, 0x7 * 4, &bpan3_pagetestmintimer_flds},
    {"txbasepagelo", NULL, 0x10 * 4, &bpan3_txbasepagelo_flds},
    {"txbasepagemid", NULL, 0x11 * 4, &bpan3_txbasepagemid_flds},
    {"txbasepagehi", NULL, 0x12 * 4, &bpan3_txbasepagehi_flds},
    {"rxbasepagelo", NULL, 0x13 * 4, &bpan3_rxbasepagelo_flds},
    {"rxbasepagemid", NULL, 0x14 * 4, &bpan3_rxbasepagemid_flds},
    {"rxbasepagehi", NULL, 0x15 * 4, &bpan3_rxbasepagehi_flds},
    {"txnextpagelo", NULL, 0x16 * 4, &bpan3_txnextpagelo_flds},
    {"txnextpagemid", NULL, 0x17 * 4, &bpan3_txnextpagemid_flds},
    {"txnextpagehi", NULL, 0x18 * 4, &bpan3_txnextpagehi_flds},
    {"rxnextpagelo", NULL, 0x19 * 4, &bpan3_rxnextpagelo_flds},
    {"rxnextpagemid", NULL, 0x1a * 4, &bpan3_rxnextpagemid_flds},
    {"rxnextpagehi", NULL, 0x1b * 4, &bpan3_rxnextpagehi_flds},
    {"fint0", NULL, 0x70 * 4, &bpan3_fint0_flds},
    {"intstatus", NULL, 0xf0 * 4, &bpan3_intstatus_flds},
};
cmd_arg_t bpan3_list = {22, bpan3_regs};

cmd_arg_item_t serdesmux_regs[] = {
    {"serdessigokovrd", NULL, 0x0 * 4, &serdesmux_serdessigokovrd_flds},
    {"serdeslpbk", NULL, 0x1 * 4, &serdesmux_serdeslpbk_flds},
    {"laneremaprx0", NULL, 0x2 * 4, &serdesmux_laneremaprx0_flds},
    {"laneremaptx0", NULL, 0x82 * 4, &serdesmux_laneremaptx0_flds},
    {"fifosts0", NULL, 0x30 * 4, &serdesmux_fifosts0_flds},
    {"fifosts1", NULL, 0x31 * 4, &serdesmux_fifosts1_flds},
    {"fifosts2", NULL, 0x32 * 4, &serdesmux_fifosts2_flds},
    {"fifosts3", NULL, 0x33 * 4, &serdesmux_fifosts3_flds},
    {"fifosts4", NULL, 0x34 * 4, &serdesmux_fifosts4_flds},
    {"fifosts5", NULL, 0x35 * 4, &serdesmux_fifosts5_flds},
    {"fifosts6", NULL, 0x36 * 4, &serdesmux_fifosts6_flds},
    {"fifosts7", NULL, 0x37 * 4, &serdesmux_fifosts7_flds},
    {"fifoerrsts", NULL, 0x40 * 4, &serdesmux_fifoerrsts_flds},
};
cmd_arg_t serdesmux_list = {13, serdesmux_regs};

cmd_arg_item_t lld_comira_regs[] = {
    {"interrupts0", &interrupts0_list, 0x0 * 4, NULL},
    {"glbl", &glbl_list, 0x1f00 * 4, NULL},
    {"stats0", &stats0_list, 0x2000 * 4, NULL},
    {"mcmac0", &mcmac0_list, 0x2100 * 4, NULL},
    {"mcmac1", &mcmac1_list, 0x2200 * 4, NULL},
    {"mcmac2", &mcmac2_list, 0x2300 * 4, NULL},
    {"mcmac3", &mcmac3_list, 0x2400 * 4, NULL},
    {"fifoctrl0", &fifoctrl0_list, 0x3f00 * 4, NULL},
    {"hsmcpcs0", &hsmcpcs0_list, 0x4000 * 4, NULL},
    {"hsmcpcs1", &hsmcpcs1_list, 0x4100 * 4, NULL},
    {"hsmcpcs2", &hsmcpcs2_list, 0x4200 * 4, NULL},
    {"hsmcpcs3", &hsmcpcs3_list, 0x4300 * 4, NULL},
    {"fecrs0", &fecrs0_list, 0x4400 * 4, NULL},
    {"fecrs1", &fecrs1_list, 0x4500 * 4, NULL},
    {"fecrs2", &fecrs2_list, 0x4600 * 4, NULL},
    {"fecrs3", &fecrs3_list, 0x4700 * 4, NULL},
    {"fecfc0", &fecfc0_list, 0x4800 * 4, NULL},
    {"fecfc1", &fecfc1_list, 0x4900 * 4, NULL},
    {"fecfc2", &fecfc2_list, 0x4a00 * 4, NULL},
    {"fecfc3", &fecfc3_list, 0x4b00 * 4, NULL},
    {"lsmcpcs0", &lsmcpcs0_list, 0x4c00 * 4, NULL},
    {"lsmcpcs1", &lsmcpcs1_list, 0x4d00 * 4, NULL},
    {"lsmcpcs2", &lsmcpcs2_list, 0x4e00 * 4, NULL},
    {"lsmcpcs3", &lsmcpcs3_list, 0x4f00 * 4, NULL},
    {"bpan0", &bpan0_list, 0x5000 * 4, NULL},
    {"bpan1", &bpan1_list, 0x5100 * 4, NULL},
    {"bpan2", &bpan2_list, 0x5200 * 4, NULL},
    {"bpan3", &bpan3_list, 0x5300 * 4, NULL},
    {"serdesmux", &serdesmux_list, 0x5f00 * 4, NULL},
};
cmd_arg_t comira_list = {29, lld_comira_regs};
