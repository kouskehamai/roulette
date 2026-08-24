"""
カスタムルーレット - Python(Tkinter)版

TkinterはPython標準ライブラリに最初から含まれるGUIツールキット。
追加のインストール不要で、pythonコマンドがあればすぐ動く
(コンパイルという工程自体がなく、ソースをそのままインタプリタが実行する)。

角丸ボタンまでは作り込まず、フラットな配色でWeb版に寄せている。
"""

import random
import tkinter as tk
from tkinter import font as tkfont
from tkinter import messagebox

# ---- Web版に合わせた配色 ----
COLOR_BG = "#f4f4f9"
COLOR_TEXT_DARK = "#333333"
COLOR_ROULETTE = "#ff6347"
COLOR_BTN_NORMAL = "#007bff"
COLOR_BTN_HOVER = "#0056b3"
COLOR_BTN_DISABLED = "#aaaaaa"


class RouletteApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("カスタムルーレット")
        self.root.geometry("500x640")
        self.root.resizable(False, False)
        self.root.configure(bg=COLOR_BG)
        try:
            self.root.iconbitmap("icon.ico")
        except tk.TclError:
            pass  # アイコンが無くても動作は継続する

        self.values: list[str] = []
        self.running = False
        self.timer_id: str | None = None

        font_title = tkfont.Font(family="Yu Gothic UI", size=20, weight="bold")
        self.font_ui = tkfont.Font(family="Yu Gothic UI", size=12)
        font_roulette = tkfont.Font(family="Yu Gothic UI", size=32, weight="bold")

        # 見出し
        tk.Label(
            root, text="カスタムルーレット", font=font_title,
            fg=COLOR_TEXT_DARK, bg=COLOR_BG,
        ).pack(pady=(20, 20))

        # 入力欄 + 追加ボタン
        input_frame = tk.Frame(root, bg=COLOR_BG)
        input_frame.pack(fill="x", padx=24)
        self.entry = tk.Entry(input_frame, font=self.font_ui)
        self.entry.pack(side="left", fill="x", expand=True, ipady=4)
        self.entry.bind("<Return>", lambda _e: self.add_value())

        self.add_btn = self._make_button(input_frame, "追加", self.add_value)
        self.add_btn.pack(side="right", padx=(10, 0))

        # 候補リスト
        list_frame = tk.Frame(root, bg=COLOR_BG)
        list_frame.pack(fill="both", padx=24, pady=(14, 0))
        self.listbox = tk.Listbox(list_frame, font=self.font_ui, height=7)
        self.listbox.pack(fill="both", expand=True)

        # 削除ボタン
        self.delete_btn = self._make_button(root, "削除", self.delete_selected)
        self.delete_btn.pack(anchor="w", padx=24, pady=(14, 20))

        # ルーレット表示
        self.roulette_label = tk.Label(
            root, text="-", font=font_roulette, fg=COLOR_ROULETTE, bg=COLOR_BG,
        )
        self.roulette_label.pack(pady=20, expand=True)

        # スタート・ストップボタン
        button_row = tk.Frame(root, bg=COLOR_BG)
        button_row.pack(pady=20)
        self.start_btn = self._make_button(button_row, "スタート", self.start_roulette, width=10)
        self.start_btn.pack(side="left", padx=10)
        self._set_enabled(self.start_btn, False)

        self.stop_btn = self._make_button(button_row, "ストップ", self.stop_roulette, width=10)
        self.stop_btn.pack(side="left", padx=10)
        self._set_enabled(self.stop_btn, False)

        # キーボードショートカット(Alt+キー)。操作性向上とテスト自動化の両方に有用
        self.root.bind("<Alt-a>", lambda _e: self.add_value())
        self.root.bind("<Alt-d>", lambda _e: self.delete_selected())
        self.root.bind("<Alt-s>", lambda _e: self.start_roulette())
        self.root.bind("<Alt-t>", lambda _e: self.stop_roulette())

        # after(100, ...)による単発フォーカスだと、起動が遅い環境(exe化した場合など)で
        # ウィンドウがまだOSからフォーカスされておらず入力を受け付けられないことがあるため、
        # ウィンドウが実際にフォーカスを得たタイミングで確実に入力欄へフォーカスする
        self.root.bind("<FocusIn>", self._on_window_focus_in)

    def _on_window_focus_in(self, event):
        if event.widget == self.root:
            self.entry.focus_set()

    # ---- ボタン生成/状態管理のヘルパー ----

    def _make_button(self, parent, text, command, width=None) -> tk.Button:
        btn = tk.Button(
            parent, text=text, command=command, font=self.font_ui,
            bg=COLOR_BTN_NORMAL, fg="white",
            activebackground=COLOR_BTN_HOVER, activeforeground="white",
            relief="flat", bd=0, padx=14, pady=8, cursor="hand2",
        )
        if width:
            btn.config(width=width)
        btn.bind("<Enter>", lambda _e: self._on_hover(btn, True))
        btn.bind("<Leave>", lambda _e: self._on_hover(btn, False))
        return btn

    def _on_hover(self, btn: tk.Button, hover: bool):
        if str(btn["state"]) == "disabled":
            return
        btn.config(bg=COLOR_BTN_HOVER if hover else COLOR_BTN_NORMAL)

    def _set_enabled(self, btn: tk.Button, enabled: bool):
        btn.config(
            state="normal" if enabled else "disabled",
            bg=COLOR_BTN_NORMAL if enabled else COLOR_BTN_DISABLED,
        )

    # ---- アプリのロジック ----

    def add_value(self):
        value = self.entry.get().strip()

        if not value:
            messagebox.showwarning("カスタムルーレット", "空の値は追加できません")
            return

        if value in self.values:
            messagebox.showwarning("カスタムルーレット", "同じ値は追加できません")
            return

        self.values.append(value)
        self.listbox.insert("end", value)
        self.entry.delete(0, "end")
        self.entry.focus_set()
        self._refresh_start_button()

    def delete_selected(self):
        selection = self.listbox.curselection()
        if not selection:
            return
        index = selection[0]
        self.listbox.delete(index)
        del self.values[index]
        self._refresh_start_button()

    def _refresh_start_button(self):
        self._set_enabled(self.start_btn, bool(self.values) and not self.running)

    def start_roulette(self):
        if not self.values:
            return
        self.running = True
        self._set_enabled(self.start_btn, False)
        self._set_enabled(self.stop_btn, True)
        self._tick()

    def _tick(self):
        if not self.running:
            return
        self.roulette_label.config(text=random.choice(self.values))
        self.timer_id = self.root.after(100, self._tick)  # 0.1秒ごとに更新

    def stop_roulette(self):
        self.running = False
        if self.timer_id is not None:
            self.root.after_cancel(self.timer_id)
            self.timer_id = None
        self._set_enabled(self.stop_btn, False)
        self._refresh_start_button()


if __name__ == "__main__":
    root = tk.Tk()
    RouletteApp(root)
    root.mainloop()
