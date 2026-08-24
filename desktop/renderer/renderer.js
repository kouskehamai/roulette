const valueInput = document.getElementById("valueInput");
const addButton = document.getElementById("addButton");
const valueList = document.getElementById("valueList");
const rouletteElement = document.getElementById("roulette");
const startButton = document.getElementById("startButton");
const stopButton = document.getElementById("stopButton");

const STORAGE_KEY = "roulette-values";

// ユーザーが追加した値を保持する配列（前回終了時の内容を復元）
let values = loadValues();
let interval;

updateValueList();

function loadValues() {
    try {
        const raw = localStorage.getItem(STORAGE_KEY);
        const parsed = raw ? JSON.parse(raw) : [];
        return Array.isArray(parsed) ? parsed : [];
    } catch (e) {
        return [];
    }
}

function saveValues() {
    try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify(values));
    } catch (e) {
        // ストレージが使えない環境でも動作は継続する
    }
}

// 値をリストに追加する関数
function addValue() {
    const value = valueInput.value.trim(); // 入力値をトリム（前後の空白を削除）

    if (value === "") {
        alert("空の値は追加できません");
        return;
    }

    if (values.includes(value)) {
        alert("同じ値は追加できません");
        return;
    }

    values.push(value);
    saveValues();
    updateValueList();
    valueInput.value = ""; // 入力フィールドをリセット
    valueInput.focus();
    startButton.disabled = values.length === 0; // スタートボタンの有効化
}

// 値リストを更新する関数
function updateValueList() {
    valueList.innerHTML = ""; // リストを初期化

    if (values.length === 0) {
        valueList.textContent = "選択された値: なし";
        startButton.disabled = true;
        return;
    }

    valueList.textContent = "選択された値: ";
    values.forEach((val, index) => {
        const span = document.createElement("span");
        span.textContent = val;

        // 削除ボタン
        const deleteButton = document.createElement("button");
        deleteButton.textContent = "×";
        deleteButton.addEventListener("click", () => removeValue(index));

        span.prepend(deleteButton); // ボタンを値の前に配置
        valueList.appendChild(span);
    });
}

// 値を削除する関数
function removeValue(index) {
    values.splice(index, 1); // 指定されたインデックスの値を削除
    saveValues();
    updateValueList(); // リストを更新
}

// ルーレットをスタートする関数
function startRoulette() {
    startButton.disabled = true;
    stopButton.disabled = false;

    interval = setInterval(() => {
        const randomIndex = Math.floor(Math.random() * values.length);
        rouletteElement.textContent = values[randomIndex];
    }, 100); // 0.1秒ごとに更新
}

// ルーレットをストップする関数
function stopRoulette() {
    clearInterval(interval);
    startButton.disabled = false;
    stopButton.disabled = true;
}

// ボタンのイベントリスナーを設定
addButton.addEventListener("click", addValue);
startButton.addEventListener("click", startRoulette);
stopButton.addEventListener("click", stopRoulette);
valueInput.addEventListener("keydown", (e) => {
    if (e.key === "Enter") addValue();
});
