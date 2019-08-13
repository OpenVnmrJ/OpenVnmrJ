
import javax.swing.JOptionPane;

public class dialog {
    public static void main(String[] args) {

        if (args.length == 0)
           JOptionPane.showMessageDialog(null, "");
        else if (args.length == 1)
           JOptionPane.showMessageDialog(null, args[0]);
        else
        {
           String m = JOptionPane.showInputDialog(args[0]);
           System.out.println(m);
        }
    }
}
