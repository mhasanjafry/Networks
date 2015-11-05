import java.io.*;
import java.lang.*;
import java.util.concurrent.CopyOnWriteArrayList;

public class ParseMessage {
		String Destination;
		String FMValue;
		String ToValue;
		int HCVal;
		int NPVal;
		int msgNumber;
		String VLNode;
		String DATA_Val;
		String RegisteredName;
//		ArrayList<Node> arr;
		CopyOnWriteArrayList<Node> arr = null;

		public ParseMessage(String msg) {
			FMValue = msg.substring(3,5); // first one is starting index,last one is end+1
			ToValue = msg.substring(9,11); //
			NPVal = Character.getNumericValue(msg.charAt(15));
			HCVal = Character.getNumericValue(msg.charAt(20));
			msgNumber = Integer.parseInt(msg.substring(25,28));
			int i = msg.indexOf("DATA");
			int j = msg.indexOf("VL");
			if (i != (j+4)){
				VLNode = msg.substring(j+3,i);
			}
			else
				VLNode = "";
			DATA_Val = msg.substring(i+5);
			if (NPVal==2){
				int k = msg.indexOf("registered as");
				RegisteredName = "" + msg.charAt(k+14) + msg.charAt(k+15);
			}
			if (NPVal==6){
				String str = new String(DATA_Val);
				arr = new CopyOnWriteArrayList<>();
				int m = 0;
				do{
					int k = str.indexOf(",");
					arr.add(new Node(str.substring(m,k)));
					str = str.substring(k+1);
				} while (str.indexOf(",") != -1);
				arr.add(new Node(str));
			}
		}

		CopyOnWriteArrayList<Node> getRegistry(){
			return arr;
		}

		String getRegisteredName(){
			return RegisteredName;
		}

		String getSource(){
			return FMValue;
		}

		String getDest(){
			return ToValue;
		}

		int getNP(){
			return NPVal;
		}

		int getHC(){
			return HCVal;
		}

		int getMsgNumber(){
			return msgNumber;
		}

		String getVL(){
			return VLNode;
		}

		String getData(){
			return DATA_Val;
		}
}