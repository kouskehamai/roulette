import javax.swing.*;
import javax.swing.border.EmptyBorder;
import java.awt.*;
import java.awt.event.*;
import java.util.List;
import java.util.Random;

/**
 * カスタムルーレット - Java(Swing)版
 *
 * JDK標準搭載のSwingを使用。追加ライブラリのダウンロードは不要。
 * ボタンはpaintComponentをオーバーライドして角丸・ホバー効果を自前描画し、
 * Web版の見た目(青いボタン、トマト色のルーレット表示)に寄せている。
 */
public class CustomRoulette extends JFrame {

    // ---- Web版に合わせた配色 ----
    private static final Color COLOR_BG = new Color(0xf4, 0xf4, 0xf9);
    private static final Color COLOR_TEXT_DARK = new Color(0x33, 0x33, 0x33);
    private static final Color COLOR_ROULETTE = new Color(0xff, 0x63, 0x47);
    private static final Color COLOR_BTN_NORMAL = new Color(0x00, 0x7b, 0xff);
    private static final Color COLOR_BTN_HOVER = new Color(0x00, 0x56, 0xb3);
    private static final Color COLOR_BTN_DISABLED = new Color(0xaa, 0xaa, 0xaa);

    private static final Font FONT_TITLE = new Font("Yu Gothic UI", Font.BOLD, 28);
    private static final Font FONT_UI = new Font("Yu Gothic UI", Font.PLAIN, 16);
    private static final Font FONT_ROULETTE = new Font("Yu Gothic UI", Font.BOLD, 48);

    private final DefaultListModel<String> listModel = new DefaultListModel<>();
    private final JList<String> valueList = new JList<>(listModel);
    private final JTextField input = new JTextField();
    private final JLabel rouletteLabel = new JLabel("-", SwingConstants.CENTER);

    private RoundButton addButton, deleteButton, startButton, stopButton;
    private final Random random = new Random();
    private Timer timer;
    private boolean running = false;

    public CustomRoulette() {
        super("カスタムルーレット");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(500, 660);
        setLocationRelativeTo(null);
        setResizable(false);

        ImageIcon appIcon = new ImageIcon(getClass().getResource("/icon.png"));
        setIconImage(appIcon.getImage());

        JPanel root = new JPanel();
        root.setLayout(new BoxLayout(root, BoxLayout.Y_AXIS));
        root.setBackground(COLOR_BG);
        root.setBorder(new EmptyBorder(20, 24, 20, 24));
        setContentPane(root);

        // 見出し
        JLabel title = new JLabel("カスタムルーレット");
        title.setFont(FONT_TITLE);
        title.setForeground(COLOR_TEXT_DARK);
        title.setAlignmentX(Component.CENTER_ALIGNMENT);
        title.setBorder(new EmptyBorder(0, 0, 20, 0));
        root.add(title);

        // 入力欄 + 追加ボタン
        JPanel inputRow = new JPanel(new BorderLayout(10, 0));
        inputRow.setOpaque(false);
        inputRow.setMaximumSize(new Dimension(Integer.MAX_VALUE, 34));
        input.setFont(FONT_UI);
        addButton = new RoundButton("追加");
        addButton.setPreferredSize(new Dimension(90, 32));
        addButton.setMnemonic('A');
        addButton.addActionListener(e -> addValue());
        input.addActionListener(e -> addValue()); // Enterキーで追加
        inputRow.add(input, BorderLayout.CENTER);
        inputRow.add(addButton, BorderLayout.EAST);
        root.add(inputRow);
        root.add(Box.createVerticalStrut(14));

        // 候補リスト
        valueList.setFont(FONT_UI);
        JScrollPane scrollPane = new JScrollPane(valueList);
        scrollPane.setMaximumSize(new Dimension(Integer.MAX_VALUE, 150));
        scrollPane.setPreferredSize(new Dimension(452, 150));
        root.add(scrollPane);
        root.add(Box.createVerticalStrut(14));

        // 削除ボタン
        deleteButton = new RoundButton("削除");
        deleteButton.setPreferredSize(new Dimension(100, 32));
        deleteButton.setAlignmentX(Component.LEFT_ALIGNMENT);
        deleteButton.setMaximumSize(new Dimension(100, 32));
        deleteButton.setMnemonic('D');
        deleteButton.addActionListener(e -> deleteSelected());
        JPanel deleteRow = new JPanel(new FlowLayout(FlowLayout.LEFT, 0, 0));
        deleteRow.setOpaque(false);
        deleteRow.setMaximumSize(new Dimension(Integer.MAX_VALUE, 32));
        deleteRow.add(deleteButton);
        root.add(deleteRow);
        root.add(Box.createVerticalStrut(20));

        // ルーレット表示
        rouletteLabel.setFont(FONT_ROULETTE);
        rouletteLabel.setForeground(COLOR_ROULETTE);
        rouletteLabel.setAlignmentX(Component.CENTER_ALIGNMENT);
        rouletteLabel.setMaximumSize(new Dimension(Integer.MAX_VALUE, 110));
        rouletteLabel.setPreferredSize(new Dimension(452, 110));
        root.add(rouletteLabel);
        root.add(Box.createVerticalGlue());

        // スタート・ストップボタン
        JPanel buttonRow = new JPanel(new FlowLayout(FlowLayout.CENTER, 20, 0));
        buttonRow.setOpaque(false);
        buttonRow.setMaximumSize(new Dimension(Integer.MAX_VALUE, 44));
        startButton = new RoundButton("スタート");
        startButton.setPreferredSize(new Dimension(110, 40));
        startButton.setEnabled(false);
        startButton.setMnemonic('S');
        startButton.addActionListener(e -> startRoulette());
        stopButton = new RoundButton("ストップ");
        stopButton.setPreferredSize(new Dimension(110, 40));
        stopButton.setEnabled(false);
        stopButton.setMnemonic('T');
        stopButton.addActionListener(e -> stopRoulette());
        buttonRow.add(startButton);
        buttonRow.add(stopButton);
        root.add(buttonRow);

        // requestFocusInWindow()はウィンドウが実際にフォーカスを得るまで効かないため、
        // ウィンドウがフォーカスを得たタイミングで確実に入力欄へフォーカスする
        addWindowFocusListener(new WindowAdapter() {
            @Override
            public void windowGainedFocus(WindowEvent e) {
                input.requestFocusInWindow();
            }
        });
        setVisible(true);
    }

    private void addValue() {
        String value = input.getText().trim();

        if (value.isEmpty()) {
            JOptionPane.showMessageDialog(this, "空の値は追加できません",
                    "カスタムルーレット", JOptionPane.WARNING_MESSAGE);
            return;
        }

        List<String> values = java.util.Collections.list(listModel.elements());
        if (values.contains(value)) {
            JOptionPane.showMessageDialog(this, "同じ値は追加できません",
                    "カスタムルーレット", JOptionPane.WARNING_MESSAGE);
            return;
        }

        listModel.addElement(value);
        input.setText("");
        input.requestFocusInWindow();
        startButton.setEnabled(!running && listModel.size() > 0);
    }

    private void deleteSelected() {
        int idx = valueList.getSelectedIndex();
        if (idx >= 0) {
            listModel.remove(idx);
            startButton.setEnabled(!running && listModel.size() > 0);
        }
    }

    private void startRoulette() {
        if (listModel.isEmpty()) return;
        running = true;
        startButton.setEnabled(false);
        stopButton.setEnabled(true);

        timer = new Timer(100, e -> {
            int idx = random.nextInt(listModel.size());
            rouletteLabel.setText(listModel.get(idx));
        });
        timer.start();
    }

    private void stopRoulette() {
        running = false;
        if (timer != null) timer.stop();
        startButton.setEnabled(listModel.size() > 0);
        stopButton.setEnabled(false);
    }

    /** 角丸・配色・ホバー効果を持つカスタムボタン */
    private static class RoundButton extends JButton {
        private boolean hover = false;

        RoundButton(String text) {
            super(text);
            setFont(FONT_UI);
            setForeground(Color.WHITE);
            setContentAreaFilled(false);
            setFocusPainted(false);
            setBorderPainted(false);
            setCursor(new Cursor(Cursor.HAND_CURSOR));
            addMouseListener(new MouseAdapter() {
                @Override
                public void mouseEntered(MouseEvent e) {
                    hover = true;
                    repaint();
                }

                @Override
                public void mouseExited(MouseEvent e) {
                    hover = false;
                    repaint();
                }
            });
        }

        @Override
        protected void paintComponent(Graphics g) {
            Graphics2D g2 = (Graphics2D) g.create();
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

            Color bg = !isEnabled() ? COLOR_BTN_DISABLED : (hover ? COLOR_BTN_HOVER : COLOR_BTN_NORMAL);
            g2.setColor(bg);
            g2.fillRoundRect(0, 0, getWidth(), getHeight(), 14, 14);
            g2.dispose();

            super.paintComponent(g);
        }
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(CustomRoulette::new);
    }
}
