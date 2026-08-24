package main

import (
	"image/color"
	"math/rand"
	"net/http"
	"os"
	"time"

	"fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/canvas"
	"fyne.io/fyne/v2/container"
	"fyne.io/fyne/v2/dialog"
	"fyne.io/fyne/v2/theme"
	"fyne.io/fyne/v2/widget"
)

// ---- Web版に合わせた配色 ----
var (
	colorBG        = color.NRGBA{R: 0xf4, G: 0xf4, B: 0xf9, A: 0xff}
	colorTextDark  = color.NRGBA{R: 0x33, G: 0x33, B: 0x33, A: 0xff}
	colorRoulette  = color.NRGBA{R: 0xff, G: 0x63, B: 0x47, A: 0xff}
	colorBtnNormal = color.NRGBA{R: 0x00, G: 0x7b, B: 0xff, A: 0xff}
)

// customTheme はFyneの既定テーマを上書きし、Web版と同じ配色にする
type customTheme struct {
	fyne.Theme
}

func (m customTheme) Color(name fyne.ThemeColorName, variant fyne.ThemeVariant) color.Color {
	switch name {
	case theme.ColorNameButton, theme.ColorNamePrimary:
		return colorBtnNormal
	case theme.ColorNameBackground:
		return colorBG
	case theme.ColorNameForeground:
		return colorTextDark
	case theme.ColorNameInputBackground:
		return color.White
	case theme.ColorNameDisabledButton:
		return color.NRGBA{R: 0xaa, G: 0xaa, B: 0xaa, A: 0xff}
	case theme.ColorNameDisabled:
		return color.White
	}
	return m.Theme.Color(name, variant)
}

func main() {
	a := app.NewWithID("com.kousukehamai.customroulette")
	a.Settings().SetTheme(&customTheme{Theme: theme.DefaultTheme()})

	w := a.NewWindow("カスタムルーレット")
	w.Resize(fyne.NewSize(500, 640))
	w.SetFixedSize(true)

	if data, err := os.ReadFile("icon.ico"); err == nil {
		w.SetIcon(fyne.NewStaticResource("icon.ico", data))
	}

	values := []string{}
	running := false
	stopTimer := make(chan struct{})

	title := canvas.NewText("カスタムルーレット", colorTextDark)
	title.TextSize = 28
	title.TextStyle = fyne.TextStyle{Bold: true}
	title.Alignment = fyne.TextAlignCenter

	rouletteText := canvas.NewText("-", colorRoulette)
	rouletteText.TextSize = 48
	rouletteText.TextStyle = fyne.TextStyle{Bold: true}
	rouletteText.Alignment = fyne.TextAlignCenter

	entry := widget.NewEntry()

	list := widget.NewList(
		func() int { return len(values) },
		func() fyne.CanvasObject { return widget.NewLabel("") },
		func(id widget.ListItemID, obj fyne.CanvasObject) {
			obj.(*widget.Label).SetText(values[id])
		},
	)

	var startBtn, stopBtn *widget.Button

	refreshList := func() {
		list.Refresh()
	}

	refreshStartButton := func() {
		if len(values) > 0 && !running {
			startBtn.Enable()
		} else {
			startBtn.Disable()
		}
	}

	addValue := func() {
		value := entry.Text
		if value == "" {
			dialog.ShowInformation("カスタムルーレット", "空の値は追加できません", w)
			return
		}
		for _, v := range values {
			if v == value {
				dialog.ShowInformation("カスタムルーレット", "同じ値は追加できません", w)
				return
			}
		}
		values = append(values, value)
		refreshList()
		entry.SetText("")
		w.Canvas().Focus(entry)
		refreshStartButton()
	}
	entry.OnSubmitted = func(string) { addValue() }

	addBtn := widget.NewButton("追加", addValue)

	var selectedIndex = -1
	list.OnSelected = func(id widget.ListItemID) { selectedIndex = id }
	list.OnUnselected = func(widget.ListItemID) { selectedIndex = -1 }

	deleteBtn := widget.NewButton("削除", func() {
		if selectedIndex < 0 || selectedIndex >= len(values) {
			return
		}
		values = append(values[:selectedIndex], values[selectedIndex+1:]...)
		selectedIndex = -1
		refreshList()
		refreshStartButton()
	})

	tick := func() {
		ticker := time.NewTicker(100 * time.Millisecond) // 0.1秒ごとに更新
		defer ticker.Stop()
		for {
			select {
			case <-ticker.C:
				idx := rand.Intn(len(values))
				fyne.Do(func() { rouletteText.Text = values[idx]; rouletteText.Refresh() })
			case <-stopTimer:
				return
			}
		}
	}

	startBtn = widget.NewButton("スタート", func() {
		if len(values) == 0 {
			return
		}
		running = true
		startBtn.Disable()
		stopBtn.Enable()
		go tick()
	})
	startBtn.Disable()

	stopBtn = widget.NewButton("ストップ", func() {
		running = false
		stopTimer <- struct{}{}
		stopBtn.Disable()
		refreshStartButton()
	})
	stopBtn.Disable()

	inputRow := container.NewBorder(nil, nil, nil, addBtn, entry)

	buttonRow := container.NewCenter(container.NewHBox(startBtn, stopBtn))

	content := container.NewBorder(
		container.NewVBox(
			container.NewPadded(title),
			inputRow,
			container.New(&fixedHeight{height: 150}, list),
			container.NewHBox(deleteBtn),
		),
		container.NewPadded(buttonRow),
		nil, nil,
		container.NewCenter(rouletteText),
	)

	w.SetContent(container.NewPadded(content))
	w.ShowAndRun()

	// http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
	// 	w.Write([]byte("Hello hello"))
	// })
	http.ListenAndServe("127.0.0.1:8080", nil)
}

// fixedHeight はリストボックスの高さを固定するための最小限のレイアウト
type fixedHeight struct {
	height float32
}

func (f *fixedHeight) MinSize(objects []fyne.CanvasObject) fyne.Size {
	return fyne.NewSize(40, f.height)
}

func (f *fixedHeight) Layout(objects []fyne.CanvasObject, size fyne.Size) {
	for _, o := range objects {
		o.Resize(fyne.NewSize(size.Width, f.height))
	}
}
