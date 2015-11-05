import java.io.*;
import java.lang.*;

public class Node{
	String NodeNum; 
	String IP;
	String Port;

	public Node(String msg){
		NodeNum = msg.substring(0,2);
		int k = msg.indexOf("@");
		IP = msg.substring(3,k);
		Port = msg.substring(k+1);
		System.out.println("Node ID: "+NodeNum+", IP Address: "+IP+", with port: "+Port);
	}

	String getNode(){
		return NodeNum;
	}

	String getIP(){
		return IP;
	}

	String getPort(){
		return Port;
	}
}