/* 他のファイルで作った関数がありますと C compiler に教える */
void io_hlt(void);

void HariMain(void)
{
    while (1) {
        io_hlt();
    }
}
