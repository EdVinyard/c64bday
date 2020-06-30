disk: $(PROGRAM)
	c1541 -format diskname,id d64 $(PROGRAM).d64 -attach $(PROGRAM).d64 -write $(PROGRAM) birthday