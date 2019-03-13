
interface CLibrary extends Library {
    CLibrary INSTANCE = (CLibrary) Native.loadLibrary("c", CLibrary.class);
   	int getpid ();
}

public class test {

    public static void main(String[] args) throws InterruptedException {
		System.out.println("Hi there!");
		while(true) {
			Thread.sleep(1000);
			System.out.println(CLibrary.INSTANCE.getpid());
		}
	}

}
