    MULU.L  D7,D1       ;45
    DIVS.L #$10000,D3   ;96

    MOVE.L D4,(A1)+     ;4/4/6
    ADD.L D4,D5         ;0/2/3
    MOVE.L (A1),-(A2)   ;6/7/9
    ADD.L D5,D6         ;0/2/3
