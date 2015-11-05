import java.io.*;
import java.lang.*;

public class SendingMessage{
	String msg; 
	String IP;
	int Port;
	int case_num;
	long time_sent = 0;
	int num_sent = 0;

	public SendingMessage(String message, String IPAddr, String Portnum, int cases){
		case_num = cases;
		msg = message;
		IP = IPAddr;
		Port = Integer.valueOf(Portnum.trim());
	}

	String getMessage(){
		return msg;
	}

	String getIP(){
		return IP;
	}

	int getPort(){
		return Port;
	}

	int getCase(){
		return case_num;
	}

	void setLastTimeSent(long time){
		time_sent = time;
	}

	long getLastTimeSent(){
		return time_sent;
	}

	int getNumSent(){
		return num_sent;
	}

	void setConfirmation(){
		num_sent = -1;
	}

	void IncreaseNumSent(){
		num_sent = num_sent + 1;
	}
}