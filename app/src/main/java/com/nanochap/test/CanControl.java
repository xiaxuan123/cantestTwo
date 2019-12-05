package com.nanochap.test;

public class CanControl {
	public native static  void InitCan(int baudrate);
	public native static  int OpenCan();
	public native static  int CanWrite(int canId,String data);
	public native static  CanFrame CanRead(CanFrame mcanFrame, int time);
	public native static   void CloseCan();
}

