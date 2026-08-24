// カスタムルーレット - Go Webサーバー版(サーバーサイドレンダリング)
//
// 前バージョンは「既存のHTMLファイルをそのまま配る」だけだったが、
// 今回はGo自身がHTMLを組み立てて返す、本来の意味での「Goで作ったWebアプリ」。
//
// 値の追加・削除はサーバー側(Go)でフォーム送信を受け取って処理する、
// 昔ながらのWebアプリの作り方(Post-Redirect-Get)。
// ルーレットの0.1秒ごとの回転表示だけは、サーバーと通信するとカクつくため
// ブラウザ側のJavaScriptで行う(値の一覧はページ読み込み時にGoから渡す)。
package main

import (
	"encoding/json"
	"html/template"
	"log"
	"net/http"
	"strconv"
	"sync"
)

// ---- アプリの状態(候補リスト)。今回は簡略化のため単一プロセス内の
//      メモリ上に保持する(サーバーを再起動すると消える)。同時アクセスに
//      備えてミューテックスで保護している。 ----
var (
	mu     sync.Mutex
	values []string
)

type pageData struct {
	Values     []string
	ValuesJSON template.JS
	HasError   bool
	ErrorMsg   string
}

var pageTmpl = template.Must(template.New("page").Parse(pageHTML))

func handleIndex(w http.ResponseWriter, r *http.Request) {
	mu.Lock()
	valuesCopy := append([]string(nil), values...)
	mu.Unlock()

	jsonBytes, _ := json.Marshal(valuesCopy)

	data := pageData{
		Values:     valuesCopy,
		ValuesJSON: template.JS(jsonBytes),
	}

	if msg := r.URL.Query().Get("error"); msg != "" {
		data.HasError = true
		data.ErrorMsg = msg
	}

	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	if err := pageTmpl.Execute(w, data); err != nil {
		log.Println("template error:", err)
	}
}

func handleAdd(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Redirect(w, r, "/", http.StatusSeeOther)
		return
	}

	value := r.FormValue("value")

	mu.Lock()
	defer mu.Unlock()

	switch {
	case value == "":
		http.Redirect(w, r, "/?error=空の値は追加できません", http.StatusSeeOther)
		return
	case contains(values, value):
		http.Redirect(w, r, "/?error=同じ値は追加できません", http.StatusSeeOther)
		return
	}

	values = append(values, value)
	http.Redirect(w, r, "/", http.StatusSeeOther)
}

func handleDelete(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Redirect(w, r, "/", http.StatusSeeOther)
		return
	}

	idx, err := strconv.Atoi(r.FormValue("index"))

	mu.Lock()
	defer mu.Unlock()

	if err == nil && idx >= 0 && idx < len(values) {
		values = append(values[:idx], values[idx+1:]...)
	}

	http.Redirect(w, r, "/", http.StatusSeeOther)
}

func contains(list []string, target string) bool {
	for _, v := range list {
		if v == target {
			return true
		}
	}
	return false
}

func main() {
	http.HandleFunc("/", handleIndex)
	http.HandleFunc("/add", handleAdd)
	http.HandleFunc("/delete", handleDelete)

	addr := "127.0.0.1:8080"
	log.Printf("起動しました: http://%s/", addr)
	log.Fatal(http.ListenAndServe(addr, nil))
}

// ---- Web版に合わせた配色・レイアウトのHTMLテンプレート ----
const pageHTML = `<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>カスタムルーレット (Go Web版)</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f4f4f9;
            margin: 0;
            padding: 0;
        }
        h1 { margin-top: 50px; color: #333; }
        .roulette {
            font-size: 80px;
            font-weight: bold;
            color: #ff6347;
            margin: 30px auto;
            height: 120px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .input-container { margin: 20px; }
        input { font-size: 18px; padding: 5px; }
        .error { color: #c00; margin: 10px; }
        .list {
            margin: 20px;
            font-size: 18px;
            color: #333;
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            justify-content: center;
        }
        .list span {
            display: flex;
            align-items: center;
            background-color: #eee;
            padding: 5px 10px;
            border-radius: 5px;
            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
        }
        .list form { display: inline; margin: 0; }
        .list span button {
            background: #ccc;
            color: #333;
            border: none;
            border-radius: 50%;
            width: 20px;
            height: 20px;
            font-size: 14px;
            cursor: pointer;
            margin-right: 10px;
        }
        button {
            padding: 10px 20px;
            font-size: 18px;
            color: #fff;
            background-color: #007bff;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            margin: 5px;
        }
        button:hover { background-color: #0056b3; }
        button:disabled { background-color: #aaa; cursor: default; }
    </style>
</head>
<body>
    <h1>カスタムルーレット (Go Web版)</h1>

    {{if .HasError}}<div class="error">{{.ErrorMsg}}</div>{{end}}

    <form class="input-container" method="POST" action="/add">
        <input type="text" name="value" placeholder="値を入力" autofocus>
        <button type="submit">追加</button>
    </form>

    <div class="list">
        {{if not .Values}}
            選択された値: なし
        {{else}}
            選択された値:
            {{range $i, $v := .Values}}
                <span>
                    <form action="/delete" method="POST">
                        <input type="hidden" name="index" value="{{$i}}">
                        <button type="submit" title="削除">×</button>
                    </form>
                    {{$v}}
                </span>
            {{end}}
        {{end}}
    </div>

    <div class="roulette" id="roulette">-</div>
    <button id="startButton" {{if not .Values}}disabled{{end}}>スタート</button>
    <button id="stopButton" disabled>ストップ</button>

    <script>
        // 値の一覧はサーバー(Go)がページ読み込み時にJSONとして埋め込む。
        // ルーレットの0.1秒ごとの切り替えは、サーバーと通信せずブラウザ内で完結させる。
        const values = {{.ValuesJSON}};
        const rouletteElement = document.getElementById("roulette");
        const startButton = document.getElementById("startButton");
        const stopButton = document.getElementById("stopButton");
        let interval;

        startButton.addEventListener("click", () => {
            if (values.length === 0) return;
            startButton.disabled = true;
            stopButton.disabled = false;
            interval = setInterval(() => {
                const randomIndex = Math.floor(Math.random() * values.length);
                rouletteElement.textContent = values[randomIndex];
            }, 100);
        });

        stopButton.addEventListener("click", () => {
            clearInterval(interval);
            startButton.disabled = false;
            stopButton.disabled = true;
        });
    </script>
</body>
</html>
`
