import java.io.*;
import java.net.*;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.Iterator;
import java.util.Scanner;

class UDPClient2 { 

	static String myName = "00";
	static int messageNum = 786;   
	static CopyOnWriteArrayList<Node> Registered_Nodes = null;
	
	public static void main(String args[]) throws Exception{       
		BufferedReader inFromUser =  new BufferedReader(new InputStreamReader(System.in));
		DatagramSocket clientSocket = new DatagramSocket();
		InetAddress IPAddress = InetAddress.getByName("dns.postel.org");       
		byte[] sendData = new byte[500];
		byte[] receiveData = new byte[500];
		String sentence = prepareMessage(1, "", "", "", 1, "", -1);
		System.out.println("Sending: "+sentence);
		sendData = sentence.getBytes();
		DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, 60450);
		clientSocket.send(sendPacket);
		int i = 0;
		DatagramPacket receivePacket = null;
		clientSocket.setSoTimeout(2000);
		ParseMessage msg = null;
		String recievemessage = null;
		while(i<5) {
	    	receivePacket = new DatagramPacket(receiveData, receiveData.length);
    		try {
				clientSocket.receive(receivePacket);
				recievemessage = new String(receivePacket.getData());
				System.out.println("Recieved: "+recievemessage);
				msg = new ParseMessage(recievemessage);
				if (msg.getNP() == 2 && myName.equals(msg.getDest())){ //else incorrect message recieved
					myName = msg.getRegisteredName();
					clientSocket.setSoTimeout(20000000);
					break;
				}
				else{
					System.out.println("incorrect message recieved");
				}
			} catch (SocketTimeoutException e) {
       			// resend
       			System.out.println("Timeout");
       			i++;
       			clientSocket.send(sendPacket);
       			continue;
    		}
		}

		Registered_Nodes = new CopyOnWriteArrayList<Node>();
		CopyOnWriteArrayList<SendingMessage> MessageArray = new CopyOnWriteArrayList<SendingMessage>();

		Recieving R2 = new Recieving("Recieving Thread", clientSocket, MessageArray);
      	R2.start();

		RunnableDemo R1 = new RunnableDemo("Sending Thread", clientSocket, MessageArray);
      	R1.start();


		Scanner in = new Scanner(System.in);
		String input;
	    Iterator<Node> iterator;
	    Node n = null;
		while (true){
			try{
				System.out.print("Type r to download registry, s to send message, b for broadcast: ");
				input = in.next();
				if (input.equals("r")){
					sentence = prepareMessage(2, "", "", "", 1, "", -1);
					MessageArray.add(new SendingMessage(sentence, "dns.postel.org", "60450", 1));
				}
				else if (input.equals("s")){
					System.out.println("Enter destination (2-digit integer from 01 to 40) from the registry:");
					String dest = in.next();
					if (!Registered_Nodes.isEmpty()){
	    				iterator = Registered_Nodes.iterator();
    					while (iterator.hasNext()){ //FM:01_TO:##_ NP:6_HC:1_NO:###
    						n = iterator.next();
    						if (dest.equals(n.getNode())){
    							System.out.println("Type your message (ending with “.”)");
								String typedmessage = "";
								while (in.hasNext()){
									typedmessage += in.next() + " ";
									if (typedmessage.indexOf(".")!=-1){
//										typedmessage = typedmessage.substring(0,typedmessage.indexOf("."));
										break;
									}
									typedmessage += " ";
								}
								sentence = prepareMessage(3, n.getNode() , "", typedmessage, 1, "", -1);
//								System.out.println("Prepared Message: "+sentence+", sending to IP and Port: "+n.getIP()+","+n.getPort());
								MessageArray.add(new SendingMessage(sentence, n.getIP() , n.getPort(), 2));
							}
						}
					}
					else
						System.out.println("List of Registered Nodes Empty!");
				}
				else if (input.equals("b")){
    				System.out.println("Type your message (ending with “.”)");
					String typedmessage = "";
					while (in.hasNext()){
						typedmessage += in.next();
						if (typedmessage.indexOf(".")!=-1){
							break;
						}
						typedmessage += " ";
					}
					if (!Registered_Nodes.isEmpty()){
						iterator = Registered_Nodes.iterator();
						while (iterator.hasNext()){
    						n = iterator.next();
    						if ((!n.getNode().equals(myName)) && (!n.getNode().equals("01"))){
	    						sentence = prepareMessage(4, n.getNode() , "", typedmessage, 1, "", -1); 
								MessageArray.add(new SendingMessage(sentence, n.getIP() , n.getPort(), 2));
							}
						}
					}
					else
						System.out.println("No nodes in Registered Nodes to broadcast to!");
				}
				else
					System.out.println("Incorrect input. Enter again!");
				Thread.sleep(2000);
			}catch (InterruptedException e) {
      			System.out.println("Main Thread interrupted.");
    		}
		}
		
//		clientSocket.close();
	}

	public static String prepareMessage(int caseNum, String Dest, String VL_Nodes, String Data, int HC, String src, int msgNumRequest){
		String Destination = "01";
		int HCVal = 1;
		String NPVal = null;
		String VLNode = "";
		String DATA_Val =  Data;
		String source = myName;
		int messageNumber = messageNum;
		if (caseNum == 1){
			NPVal = "1";
			DATA_Val =  "register me";
		}
		else if (caseNum==2){
			NPVal = "5";
			DATA_Val =  "GET MAP";
		}
		else if (caseNum==3){
			Destination = Dest;
			NPVal = "3";
		}
		else if (caseNum==4){ //NP:8_HC:1_NO:###_ VL:_DATA:xxxx...
			Destination = Dest;
			NPVal = "8";
		}
		else if (caseNum==5){
			messageNumber = msgNumRequest;
			Destination = Dest;
			HCVal = HC-1;
			VLNode = VL_Nodes + "," + myName;
			source = src;
			NPVal = "3";
		}
		else if (caseNum==6){
			messageNumber = msgNumRequest;
			Destination = Dest;
			NPVal = "4";
			DATA_Val = "OK";
		}
		else if (caseNum==7){
			messageNumber = msgNumRequest;
			Destination = Dest;
			NPVal = "9";
			DATA_Val = "OK";
		}
		String str = "FM:"+source+" TO:"+Destination+" NP:"+NPVal+" HC:"+HCVal+" NO:"+String.format("%03d", messageNumber)+" VL:"+VLNode+" DATA:"+DATA_Val;
		if (messageNumber == messageNum) messageNum++;
		return str;
	}
} 

class RunnableDemo implements Runnable {
   private Thread t;
   private String threadName;
   DatagramSocket clientSocket;
   CopyOnWriteArrayList<SendingMessage> MessageArr;
   InetAddress IPAddress;
   
   RunnableDemo(String name, DatagramSocket client, CopyOnWriteArrayList<SendingMessage> MessageArray){
       threadName = name;
       clientSocket = client;
       MessageArr = MessageArray;
   }

   public void run(){
      byte[] sendData = null;
      Iterator<SendingMessage> iterator;
      DatagramPacket sendPacket = null;
      SendingMessage msg = null;
      while (true){
	    try {
	    if (!MessageArr.isEmpty()){
	    	sendData = new byte[500];
	    	iterator = MessageArr.iterator();
    		while (iterator.hasNext()){
	    		msg = iterator.next();
	    		if (System.currentTimeMillis() - msg.getLastTimeSent() > 2000){
			      	sendData = msg.getMessage().getBytes();
		    	  	try{
		    	  		if (msg.getNumSent() > 0) System.out.println("Timeout occurred! Sending again.");
			    	  	if (msg.getCase()==1)
		    	  			IPAddress = InetAddress.getByName("dns.postel.org");
	    	  			else{
		    	  			IPAddress = InetAddress.getByName(msg.getIP());
	    	  			}
						sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, msg.getPort());
						clientSocket.send(sendPacket);
						if (msg.getNumSent() != -1){
							msg.setLastTimeSent(System.currentTimeMillis());
			      			msg.IncreaseNumSent();
							System.out.println("Sending Try "+msg.getNumSent()+": " +  msg.getMessage());
						}
						else
							System.out.println("Sending confirmation: " +  msg.getMessage());	
	      			} catch (Exception e){
	      				System.out.println("Exception occurred: " + e.getMessage());
	      			};
	      		}
				if (msg.getNumSent() == -1){
					MessageArr.remove(msg);
				}
				if (msg.getNumSent() == 5){
					System.out.println("Stopped sending as no reply recieved even after 5 tries");
					MessageArr.remove(msg);
				}
			}
		}
		Thread.sleep(1000);// Let the thread sleep for 1000 millis.
	    } catch (InterruptedException e) {
         System.out.println("Thread " +  threadName + " interrupted.");
       	}
   	}
  }
   
   public void start ()
   {
//      System.out.println("Starting " +  threadName );
      if (t == null)
      {
         t = new Thread (this, threadName);
         t.start ();
      }
   }

}

class Recieving implements Runnable {
   private Thread t;
   private String threadName;
   DatagramSocket clientSocket;
   CopyOnWriteArrayList<SendingMessage> MessageArr;
   InetAddress IPAddress;
   
   Recieving(String name, DatagramSocket client, CopyOnWriteArrayList<SendingMessage> MessageArray){
       threadName = name;
       clientSocket = client;
       MessageArr = MessageArray;
   }

   public void run(){
		byte[] receiveData = null;
		DatagramPacket receivePacket = null;
		ParseMessage msg = null;
		ParseMessage msgSent = null;
	    Iterator<SendingMessage> iterator;
		String recievemessage = null;
		String sentence = null;
//      	System.out.println("Starting " +  threadName );
	while (true){
	try{
		receiveData = new byte[2048];
      	try{   	
	    	receivePacket = new DatagramPacket(receiveData, receiveData.length);
			clientSocket.receive(receivePacket);
			recievemessage = new String(receivePacket.getData());
			System.out.println("Recieved: "+recievemessage);
			msg = new ParseMessage(recievemessage);
		}
		catch (Exception e){
			System.out.println("Exception occured in Recieving: " + e.getMessage());
		};
		int NP = msg.getNP();
		if (NP == 3){ //If a packet arrives with NP:3 that matches your registered address, print it to the screen.
			if (UDPClient2.myName.equals(msg.getDest()))
				System.out.println("Node "+msg.getSource()+" says to me: "+msg.getData());
			else{
				int HC = msg.getHC();
				if (HC == 0)
					System.out.println("Dropped – hopcount exceeded. Relayed Message: " + msg.getData());
				else if (HC > 0){ //If HC>0 
					if (msg.getVL().indexOf(UDPClient2.myName) != -1) //your node number IS in the VL list, then print out the message “dropped – node revisited” (and any message info. for the user).
						System.out.println("Dropped – node revisited. Relayed Message: " + msg.getData());
					else{
//send a relay copy to 5 nodes selected at random in the registry EXCEPT yourself and the nameserver (with HC decremented by 1,
//retaining the original TO:/FM: fields, and append ,## with your node number to VL list)
						Iterator<Node> iteratorNode;
						Node n;
						int num_relay = 0;
						if (!UDPClient2.Registered_Nodes.isEmpty()){
							iteratorNode = UDPClient2.Registered_Nodes.iterator();
							while (iteratorNode.hasNext()){
    							n = iteratorNode.next();
    							if ((!n.getNode().equals(UDPClient2.myName)) && (!n.getNode().equals("01"))){
	    							sentence = UDPClient2.prepareMessage(5, msg.getDest() , msg.getVL(), msg.getData(), HC, msg.getSource(), msg.getMsgNumber()); 
									MessageArr.add(new SendingMessage(sentence, n.getIP() , n.getPort(), 2));
									System.out.println("Relayed Message Forwarded: " + msg.getData());
									num_relay++;
								}
								if (num_relay==5)
									break;
							}
						}
						else
							System.out.println("No nodes in Registered Nodes to relay to!");
					}
				}
			}
			//send acknowledgement to the source (swap TO:/FM: identifiers, NP:4, HC:1,DATA:OK)
			sentence = UDPClient2.prepareMessage(6, msg.getSource() , "", "", 1, "", msg.getMsgNumber());
			SendingMessage confirm = new SendingMessage(sentence, receivePacket.getAddress().getHostAddress(), Integer.toString(receivePacket.getPort()), 2);
			confirm.setConfirmation();
			MessageArr.add(confirm);
		}	
		else if (UDPClient2.myName.equals(msg.getDest())){
			if (!MessageArr.isEmpty()){
				SendingMessage messageSent = null;
		    	iterator = MessageArr.iterator();
    			while (iterator.hasNext()){ //FM:01_TO:##_ NP:6_HC:1_NO:###
    				messageSent = iterator.next();
					msgSent = new ParseMessage(messageSent.getMessage()); //checked sent message 		
					if (msgSent.getMsgNumber() == msg.getMsgNumber()){
	    				if (NP == 6){ //download registry message
	    					if (msgSent.getNP() == 5){
//			    				Registered_Nodes = null;
			    				UDPClient2.Registered_Nodes = msg.getRegistry();
	    						MessageArr.remove(messageSent);		
	    					}
	    				}
	    				else if ("OK".equals(msg.getData().trim())){ //send acknowledgement to ACK
							if ((NP == 4) && (msgSent.getNP() == 3)){ 
							//get matching response with FM and TO identifiers swapped with same NO field and NP:4 and DATA:OK
								sentence = UDPClient2.prepareMessage(6, msg.getSource() , "", "", 1, "", msg.getMsgNumber());
								SendingMessage confirm = new SendingMessage(sentence, receivePacket.getAddress().getHostAddress(), Integer.toString(receivePacket.getPort()), 2);
								confirm.setConfirmation();
								MessageArr.add(confirm);
								MessageArr.remove(messageSent);
							}
	    					else if ((NP == 9) && (msgSent.getNP() == 8)){
								sentence = UDPClient2.prepareMessage(7, msg.getSource() , "", "", 1, "", msg.getMsgNumber());
								SendingMessage confirm = new SendingMessage(sentence, receivePacket.getAddress().getHostAddress(), Integer.toString(receivePacket.getPort()), 2);
								confirm.setConfirmation();
								MessageArr.add(confirm);
								MessageArr.remove(messageSent);
							}
	    				}
			    		else{
	    					System.out.println("Incorrect Message Returned: " + msg.getData());
	    				}
	    			}	    			
	    		}
	    	}
		}
	Thread.sleep(1000);// Let the thread sleep for 1000 millis.
	} catch (InterruptedException e) {
      	System.out.println("Thread " +  threadName + " interrupted.");
    }
	}
  }
   
   public void start ()
   {
//      System.out.println("Starting " +  threadName );
      if (t == null)
      {
         t = new Thread (this, threadName);
         t.start ();
      }
   }

}