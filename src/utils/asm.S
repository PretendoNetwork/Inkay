    .global __OSGetTitleVersion
__OSGetTitleVersion:
    li %r0, 0x7000
    sc
    blr
