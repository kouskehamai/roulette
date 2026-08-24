# カスタムルーレット - Ruby(Tk)版
#
# Ruby/TkはTcl/Tkへのバインディングで、Python版(Tkinter)と全く同じGUIライブラリ
# (Tcl/Tk)の上に成り立っている。同じ土台を使う言語間で、コードの書き味がどう
# 変わるかという比較になる。
#
# 角丸ボタンまでは作り込まず、フラットな配色でWeb版に寄せている(Python版と同じ方針)。

require 'tk'

# ---- Web版に合わせた配色 ----
COLOR_BG = '#f4f4f9'
COLOR_TEXT_DARK = '#333333'
COLOR_ROULETTE = '#ff6347'
COLOR_BTN_NORMAL = '#007bff'
COLOR_BTN_HOVER = '#0056b3'
COLOR_BTN_DISABLED = '#aaaaaa'

class RouletteApp
  def initialize(root)
    @root = root
    @values = []
    @running = false
    @timer = nil

    setup_window
    setup_widgets
  end

  private

  def setup_window
    @root.title('カスタムルーレット')
    @root.geometry('500x640')
    @root.resizable(false, false)
    @root['background'] = COLOR_BG
    begin
      @root.iconbitmap('icon.ico')
    rescue StandardError
      # アイコンが無くても動作は継続する
    end
  end

  def setup_widgets
    font_title = TkFont.new('family' => 'Yu Gothic UI', 'size' => 20, 'weight' => 'bold')
    @font_ui = TkFont.new('family' => 'Yu Gothic UI', 'size' => 12)
    font_roulette = TkFont.new('family' => 'Yu Gothic UI', 'size' => 32, 'weight' => 'bold')

    # 見出し
    TkLabel.new(@root) { |l|
      l.text 'カスタムルーレット'
      l.font font_title
      l.foreground COLOR_TEXT_DARK
      l.background COLOR_BG
    }.pack('pady' => [20, 20])

    # 入力欄 + 追加ボタン
    input_frame = TkFrame.new(@root) { |f| f.background COLOR_BG }
    input_frame.pack('fill' => 'x', 'padx' => 24)

    @entry = TkEntry.new(input_frame) { |e| e.font @font_ui }
    @entry.pack('side' => 'left', 'fill' => 'x', 'expand' => true, 'ipady' => 4)
    @entry.bind('Return') { add_value }

    @add_btn = make_button(input_frame, '追加') { add_value }
    @add_btn.pack('side' => 'right', 'padx' => [10, 0])

    # 候補リスト
    list_frame = TkFrame.new(@root) { |f| f.background COLOR_BG }
    list_frame.pack('fill' => 'both', 'padx' => 24, 'pady' => [14, 0])

    @listbox = TkListbox.new(list_frame) { |lb| lb.font @font_ui; lb.height 7 }
    @listbox.pack('fill' => 'both', 'expand' => true)

    # 削除ボタン
    @delete_btn = make_button(@root, '削除') { delete_selected }
    @delete_btn.pack('anchor' => 'w', 'padx' => 24, 'pady' => [14, 20])

    # ルーレット表示
    @roulette_label = TkLabel.new(@root) { |l|
      l.text '-'
      l.font font_roulette
      l.foreground COLOR_ROULETTE
      l.background COLOR_BG
    }
    @roulette_label.pack('pady' => 20, 'expand' => true)

    # スタート・ストップボタン
    button_row = TkFrame.new(@root) { |f| f.background COLOR_BG }
    button_row.pack('pady' => 20)

    @start_btn = make_button(button_row, 'スタート') { start_roulette }
    @start_btn.pack('side' => 'left', 'padx' => 10)
    set_enabled(@start_btn, false)

    @stop_btn = make_button(button_row, 'ストップ') { stop_roulette }
    @stop_btn.pack('side' => 'left', 'padx' => 10)
    set_enabled(@stop_btn, false)

    # ウィンドウが実際にフォーカスを得たタイミングで入力欄へフォーカスする
    @root.bind('FocusIn') { |event| @entry.focus if event.widget == @root }

    # キーボードショートカット(Alt+キー)。操作性向上とテスト自動化の両方に有用
    @root.bind('Alt-a') { add_value }
    @root.bind('Alt-d') { delete_selected }
    @root.bind('Alt-s') { start_roulette }
    @root.bind('Alt-t') { stop_roulette }
  end

  # ---- ボタン生成/状態管理のヘルパー ----

  def make_button(parent, label, &action)
    btn = TkButton.new(parent) { |b|
      b.text label
      b.font @font_ui
      b.foreground 'white'
      b.background COLOR_BTN_NORMAL
      b.activebackground COLOR_BTN_HOVER
      b.activeforeground 'white'
      b.relief 'flat'
      b.borderwidth 0
      b.padx 14
      b.pady 8
      b.cursor 'hand2'
      b.command(&action)
    }
    btn.bind('Enter') { on_hover(btn, true) }
    btn.bind('Leave') { on_hover(btn, false) }
    btn
  end

  def on_hover(btn, hover)
    return if btn['state'].to_s == 'disabled'

    btn['background'] = hover ? COLOR_BTN_HOVER : COLOR_BTN_NORMAL
  end

  def set_enabled(btn, enabled)
    btn['state'] = enabled ? 'normal' : 'disabled'
    btn['background'] = enabled ? COLOR_BTN_NORMAL : COLOR_BTN_DISABLED
  end

  # ---- アプリのロジック ----

  def add_value
    value = @entry.get.strip

    if value.empty?
      Tk.messageBox('type' => 'ok', 'icon' => 'warning',
                     'title' => 'カスタムルーレット', 'message' => '空の値は追加できません')
      return
    end

    if @values.include?(value)
      Tk.messageBox('type' => 'ok', 'icon' => 'warning',
                     'title' => 'カスタムルーレット', 'message' => '同じ値は追加できません')
      return
    end

    @values << value
    @listbox.insert('end', value)
    @entry.value = ''
    @entry.focus
    refresh_start_button
  end

  def delete_selected
    selection = @listbox.curselection
    return if selection.empty?

    index = selection.first
    @listbox.delete(index)
    @values.delete_at(index)
    refresh_start_button
  end

  def refresh_start_button
    set_enabled(@start_btn, !@values.empty? && !@running)
  end

  def start_roulette
    return if @values.empty?

    @running = true
    set_enabled(@start_btn, false)
    set_enabled(@stop_btn, true)
    tick
  end

  def tick
    return unless @running

    @roulette_label.text = @values.sample
    @timer = Tk.after(100) { tick } # 0.1秒ごとに更新
  end

  def stop_roulette
    @running = false
    Tk.after_cancel(@timer) if @timer
    @timer = nil
    set_enabled(@stop_btn, false)
    refresh_start_button
  end
end

root = TkRoot.new
RouletteApp.new(root)
Tk.mainloop
