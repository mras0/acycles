        move.l  d0,d3           ; d3=00Cc
        add.l   a2,d5           ; uv += duvdx
        add.w   d4,d0           ; c += dcdx
        move.b  (a0,d6.l*4),d3  ; d3=00CT
        and.l   d1,d5           ; uv &= uvmask
        move.l  d5,d6           ; d6=VvUu
        lsr.l   #8,d6           ; d6=0VvU
        move.l  d6,d2           ; d2=0VvU
        lsr.l   #8,d6           ; d6=00Vv
        move.b  d2,d6           ; d6=00VU
        move.b  (a1,d3.l),(a3)+
        dbf     d7,.loop
