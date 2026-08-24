using System.Windows;
using System.Windows.Input;
using System.Windows.Threading;

namespace CustomRoulette;

public partial class MainWindow : Window
{
    private readonly List<string> _values = new();
    private readonly Random _random = new();
    private DispatcherTimer? _timer;
    private bool _running;

    public MainWindow()
    {
        InitializeComponent();
        Loaded += (_, _) => ValueInput.Focus();
    }

    private void AddButton_Click(object sender, RoutedEventArgs e) => AddValue();

    private void ValueInput_KeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Enter) AddValue();
    }

    private void AddValue()
    {
        var value = ValueInput.Text.Trim();

        if (string.IsNullOrEmpty(value))
        {
            MessageBox.Show("空の値は追加できません", "カスタムルーレット", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }

        if (_values.Contains(value))
        {
            MessageBox.Show("同じ値は追加できません", "カスタムルーレット", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }

        _values.Add(value);
        RefreshList();
        ValueInput.Clear();
        ValueInput.Focus();
    }

    private void RefreshList()
    {
        ValueListBox.ItemsSource = null;
        ValueListBox.ItemsSource = _values;
        StartButton.IsEnabled = _values.Count > 0 && !_running;
    }

    private void DeleteButton_Click(object sender, RoutedEventArgs e)
    {
        if (ValueListBox.SelectedIndex >= 0)
        {
            _values.RemoveAt(ValueListBox.SelectedIndex);
            RefreshList();
        }
    }

    private void StartButton_Click(object sender, RoutedEventArgs e)
    {
        if (_values.Count == 0) return;

        _running = true;
        StartButton.IsEnabled = false;
        StopButton.IsEnabled = true;

        _timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(100) };
        _timer.Tick += (_, _) => RouletteText.Text = _values[_random.Next(_values.Count)];
        _timer.Start();
    }

    private void StopButton_Click(object sender, RoutedEventArgs e)
    {
        _running = false;
        _timer?.Stop();
        StartButton.IsEnabled = _values.Count > 0;
        StopButton.IsEnabled = false;
    }
}
