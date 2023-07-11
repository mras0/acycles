.loop:
        move.l  (a0),a1
        moveq   #0,d1
        moveq   #0,d2
        subq.w  #1,d0
        bne.b   .loop
