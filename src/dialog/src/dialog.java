
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JLabel;
import javax.swing.JDialog;

public class dialog {
    public static void main(String[] args) {

        if (args.length == 0)
           JOptionPane.showMessageDialog(null, "");
        else if (args.length == 1)
           JOptionPane.showMessageDialog(null, args[0]);
        else
        {
           if (args[1].startsWith("pass"))
           {
              JPanel panel = new JPanel();
              JLabel label = new JLabel(args[0]);
              JPasswordField pass = new JPasswordField(20);
              panel.add(label);
              panel.add(pass);
              String[] options = new String[]{"OK", "Cancel"};
              int option = JOptionPane.showOptionDialog(null, panel, "PassWord",
                         JOptionPane.NO_OPTION, JOptionPane.PLAIN_MESSAGE,
                         null, options, options[0]);
              if (option == 0)
              {
                 char[] password = pass.getPassword();
                 System.out.println(new String(password));
              }
              else
              {
                 System.out.println("");
              }
           }
           else
           {
               String m = JOptionPane.showInputDialog(args[0]);
               System.out.println(m);
           }
        }
    }
}

