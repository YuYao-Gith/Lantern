import javafx.application.Application;
import javafx.application.Platform;
import javafx.beans.property.SimpleStringProperty;
import javafx.concurrent.Worker;
import javafx.geometry.Insets;
import javafx.geometry.Orientation;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.layout.*;
import javafx.scene.web.WebEngine;
import javafx.scene.web.WebView;
import javafx.stage.DirectoryChooser;
import javafx.stage.Screen;
import javafx.stage.Stage;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.stream.Collectors;

import javafx.collections.FXCollections;

public class Lantern extends Application {
    private static final String WELCOME_HTML = """
            <html><head><meta charset='UTF-8'></head><body style='margin:0;background:linear-gradient(135deg,#ff6b6b,#ff4d4d);color:white;font-family:sans-serif;display:flex;flex-direction:column;justify-content:center;align-items:center;height:100vh;text-align:center'>
            <div class='lantern' style='font-size:6em;margin-bottom:20px;animation:glow 2s infinite alternate'>🏮</div>
            <h1 style='font-size:2.5em'>欢迎使用 Lantern 浏览器！</h1>
            <p style='max-width:600px;line-height:1.6'>你好呀！我是你的新伙伴 Lantern，一个轻巧又贴心的浏览器。希望我能帮你探索更广阔的世界，享受每一次点击的乐趣！</p>
            <style>@keyframes glow{from{text-shadow:0 0 10px #fff,0 0 20px #ff9999}to{text-shadow:0 0 20px #fff,0 0 40px #ff4d4d}}</style>
            </body></html>
            """;

    private TabPane tabPane;
    private HBox bookmarkBar;
    private boolean isDarkMode = false;
    private Stage primaryStage;
    private final List<Map<String, String>> historyList = new ArrayList<>();
    private final List<Map<String, String>> bookmarks = new ArrayList<>();
    private final List<DownloadTask> downloadTasks = new ArrayList<>();
    private Path lastDownloadDir = Paths.get(System.getProperty("user.home"), "Downloads");
    private final Path dataDir = Paths.get(System.getProperty("user.home"), ".lantern");
    private final Path bookmarksFile = dataDir.resolve("bookmarks.json");
    private final Path historyFile = dataDir.resolve("history.json");

    @Override
    public void init() throws Exception {
        Files.createDirectories(dataDir);
        loadBookmarks();
        loadHistory();
    }

    @Override
    public void start(Stage stage) {
        this.primaryStage = stage;
        stage.setTitle("Lantern - 你的贴心浏览伙伴");
        // 保持默认窗口装饰，尊重操作系统

        BorderPane root = new BorderPane();
        bookmarkBar = createBookmarkBar();
        root.setTop(bookmarkBar);

        tabPane = new TabPane();
        tabPane.setTabClosingPolicy(TabPane.TabClosingPolicy.ALL_TABS);
        createNewTab("data:text/html;base64," + Base64.getEncoder().encodeToString(WELCOME_HTML.getBytes(StandardCharsets.UTF_8)), false);
        root.setCenter(tabPane);

        // --- 智能设置初始窗口大小 ---
        Screen primaryScreen = Screen.getPrimary();
        var visualBounds = primaryScreen.getVisualBounds();

        double initWidth = visualBounds.getWidth() * 0.8;
        double initHeight = visualBounds.getHeight() * 0.7;

        Scene scene = new Scene(root, initWidth, initHeight);
        applyTheme(scene);
        stage.setScene(scene);
        stage.centerOnScreen(); // 居中显示
        stage.show();

        scene.addEventHandler(KeyEvent.KEY_PRESSED, e -> {
            if (e.isControlDown() && e.getCode() == KeyCode.F12) {
                if (!tabPane.getTabs().isEmpty()) {
                    WebView wv = (WebView) ((BorderPane) tabPane.getSelectionModel().getSelectedItem().getContent()).getCenter();
                    wv.getEngine().executeScript("console.log('开发者工具已激活！')");
                }
            }
        });
    }

    private HBox createBookmarkBar() {
        HBox bar = new HBox(5);
        bar.setPadding(new Insets(3));
        bar.setAlignment(Pos.CENTER_LEFT);

        updateBookmarkBar(bar);
        return bar;
    }

    private void updateBookmarkBar(HBox bar) {
        bar.getChildren().clear();
        bar.setStyle(isDarkMode ? "-fx-background-color: #2d2d2d;" : "-fx-background-color: #ffebee;");

        Button newTabBtn = new Button("+");
        newTabBtn.setOnAction(e -> createNewTab("about:blank", false));
        Button incognitoBtn = new Button("🕵️ 私密");
        incognitoBtn.setOnAction(e -> createNewTab("about:blank", true));
        Button aiBtn = new Button("🤖 AI助手");
        aiBtn.setOnAction(e -> summarizeCurrentPage());

        for (Map<String, String> bm : bookmarks) {
            Hyperlink link = new Hyperlink(bm.get("title"));
            link.setOnAction(e -> createNewTab(bm.get("url"), false));
            bar.getChildren().add(link);
        }

        // --- 不再创建任何自定义窗口控制按钮 ---

        bar.getChildren().addAll(
                new Separator(Orientation.VERTICAL),
                newTabBtn,
                incognitoBtn,
                aiBtn,
                new Separator(Orientation.VERTICAL),
                createThemeButton(),
                createHistoryButton(),
                createDownloadsButton()
                // 右侧不再有任何占位符或控制按钮
        );
    }

    private Button createThemeButton() {
        Button btn = new Button(isDarkMode ? "☀️ 日间" : "🌙 夜间");
        btn.setOnAction(e -> toggleTheme());
        return btn;
    }

    private Button createHistoryButton() {
        Button btn = new Button("📜 历史");
        btn.setOnAction(e -> showHistoryWindow());
        return btn;
    }

    private Button createDownloadsButton() {
        Button btn = new Button("⬇️ 下载");
        btn.setOnAction(e -> showDownloadsWindow());
        return btn;
    }

    private void createNewTab(String url, boolean isIncognito) {
        Tab tab = new Tab(isIncognito ? "🔒 私密窗口" : "新标签页");
        BorderPane content = new BorderPane();
        TextField urlField = new TextField(url);
        urlField.setPrefWidth(600);
        urlField.setOnAction(e -> {
            String input = urlField.getText().trim();
            if (input.startsWith("/ai ")) {
                String prompt = input.substring(4);
                callQwenAPI(prompt).thenAccept(result -> Platform.runLater(() -> {
                    try {
                        String html = "<html><head><meta charset='UTF-8'></head><body><h2>Lantern AI 回答</h2><pre>" + result + "</pre></body></html>";
                        createNewTab("data:text/html," + URLEncoder.encode(html, StandardCharsets.UTF_8), false);
                    } catch (Exception ex) {
                        showAlert("哎呀，出错了", ex.getMessage());
                    }
                }));
            } else {
                loadUrl(input, content);
            }
        });

        HBox toolbar = new HBox(5);
        toolbar.setAlignment(Pos.CENTER_LEFT);
        toolbar.setPadding(new Insets(5));
        toolbar.getChildren().addAll(
                createNavButton("◀ 返回", () -> goBack(content)),
                createNavButton("▶ 前进", () -> goForward(content)),
                createNavButton("🔄 刷新", () -> reload(content)),
                createNavButton("🏠 首页", () -> loadWelcome(content)),
                new Label(" "),
                urlField
        );

        WebView webView = new WebView();
        WebEngine engine = webView.getEngine();
        if (!isIncognito) {
            engine.setUserDataDirectory(dataDir.toFile());
        }

        if (url.endsWith(".pdf")) {
            Platform.runLater(() -> {
                try {
                    java.awt.Desktop.getDesktop().browse(new URL(url).toURI());
                } catch (Exception ex) {
                    showAlert("抱歉", "无法打开 PDF 文件");
                }
            });
        } else {
            engine.load(url);
        }

        // --- 消除 JSObject 警告 ---
        engine.getLoadWorker().stateProperty().addListener((obs, old, state) -> {
            if (state == Worker.State.SUCCEEDED) {
                try {
                    Object windowObj = engine.executeScript("window");
                    if (windowObj instanceof netscape.javascript.JSObject win) {
                        win.setMember("lantern", new LanternExtensionAPI() {
                            public void log(String msg) {
                                System.out.println("[Lantern 扩展] " + msg);
                            }
                            public void alert(String msg) {
                                Platform.runLater(() -> showAlert("来自扩展的消息", msg));
                            }
                            public String getBrowserVersion() {
                                return "Lantern 1.0";
                            }
                        });
                    }
                } catch (Exception ex) {
                    /* ignore */
                }
            }
        });

        engine.getLoadWorker().workDoneProperty().addListener((obs, old, progress) -> {
            if (progress.intValue() == 100) {
                String title = (String) engine.executeScript("document.title");
                tab.setText((isIncognito ? "🔒 " : "") + (title.length() > 30 ? title.substring(0, 30) + "..." : title));
                if (!isIncognito) addToHistory(title, engine.getLocation());
                urlField.setText(engine.getLocation());
            }
        });

        content.setTop(toolbar);
        content.setCenter(webView);
        tab.setContent(content);
        tabPane.getTabs().add(tab);
        tabPane.getSelectionModel().select(tab);
    }

    private void summarizeCurrentPage() {
        if (tabPane.getTabs().isEmpty()) return;
        BorderPane bp = (BorderPane) ((Tab)tabPane.getSelectionModel().getSelectedItem()).getContent();
        WebView wv = (WebView) bp.getCenter();
        String text = (String) wv.getEngine().executeScript("""
            (() => {
                let bodyText = document.body.innerText || '';
                return bodyText.substring(0, 3000).replace(/\\n+/g, '\\n');
            })()
            """);
        if (text.trim().isEmpty()) {
            showAlert("AI 摘要", "这个页面好像没什么内容可以总结呢！");
            return;
        }
        String prompt = "你是一个专业的内容摘要助手。请用中文为以下网页内容生成一段简洁的摘要（100字以内）：\n\n" + text;
        Alert loadingAlert = new Alert(Alert.AlertType.INFORMATION);
        loadingAlert.setTitle("Lantern AI");
        loadingAlert.setHeaderText("正在思考...");
        loadingAlert.setContentText("让我看看...（大约需要 5-10 秒）");
        loadingAlert.show();

        callQwenAPI(prompt).thenAccept(result -> Platform.runLater(() -> {
            loadingAlert.close();
            Alert resultAlert = new Alert(Alert.AlertType.INFORMATION);
            resultAlert.setTitle("Lantern AI 摘要");
            resultAlert.setHeaderText("这是我的理解：");
            resultAlert.setContentText(result);
            resultAlert.getDialogPane().setMinHeight(200);
            resultAlert.showAndWait();
        }));
    }

    private void loadUrl(String input, BorderPane parent) {
        String url = input.trim();
        if (!url.startsWith("http") && !url.startsWith("file:")) url = "https://" + url;
        WebView wv = (WebView) parent.getCenter();
        wv.getEngine().load(url);
    }

    private void goBack(BorderPane parent) {
        ((WebView)parent.getCenter()).getEngine().getHistory().go(-1);
    }

    private void goForward(BorderPane parent) {
        ((WebView)parent.getCenter()).getEngine().getHistory().go(1);
    }

    private void reload(BorderPane parent) {
        ((WebView)parent.getCenter()).getEngine().reload();
    }

    private void loadWelcome(BorderPane parent) {
        ((WebView)parent.getCenter()).getEngine().load("data:text/html;base64," + Base64.getEncoder().encodeToString(WELCOME_HTML.getBytes(StandardCharsets.UTF_8)));
    }

    private Button createNavButton(String text, Runnable action) {
        Button btn = new Button(text);
        btn.setOnAction(e -> action.run());
        return btn;
    }

    private void addToHistory(String title, String url) {
        historyList.add(0, Map.of("title", title, "url", url, "time", LocalDateTime.now().toString()));
        if (historyList.size() > 100) historyList.remove(historyList.size() - 1);
        saveHistory();
    }

    private void showHistoryWindow() {
        Stage s = new Stage();
        s.setTitle("你的浏览足迹");
        ListView<String> list = new ListView<>();
        list.setItems(FXCollections.observableArrayList(
                historyList.stream()
                        .map(h -> LocalDateTime.parse(h.get("time")).format(DateTimeFormatter.ofPattern("MM-dd HH:mm")) + " | " + h.get("title"))
                        .collect(Collectors.toList())
        ));
        list.setOnMouseClicked(e -> {
            if (e.getClickCount() == 2) {
                int i = list.getSelectionModel().getSelectedIndex();
                if (i >= 0) {
                    createNewTab(historyList.get(i).get("url"), false);
                    s.close();
                }
            }
        });
        VBox vbox = new VBox(new Label("双击任意记录即可返回"), list);
        vbox.setPadding(new Insets(10));
        s.setScene(new Scene(vbox, 500, 500));
        s.show();
    }

    private void downloadFile(String url, String defaultName) {
        DirectoryChooser dc = new DirectoryChooser();
        dc.setInitialDirectory(lastDownloadDir.toFile());
        File dir = dc.showDialog(primaryStage);
        if (dir == null) return;
        lastDownloadDir = dir.toPath();
        String filename = defaultName.isEmpty() ? url.replaceAll(".*/", "").split("\\?")[0] : defaultName;
        if (filename.isEmpty()) filename = "download.bin";
        Path target = dir.toPath().resolve(filename);
        DownloadTask task = new DownloadTask(url, target);
        downloadTasks.add(task);
        task.start();
    }

    private void showDownloadsWindow() {
        Stage s = new Stage();
        s.setTitle("下载中心");
        ListView<String> list = new ListView<>();
        list.setItems(FXCollections.observableArrayList(
                downloadTasks.stream()
                        .map(t -> t.status.get() + " - " + t.target.getFileName())
                        .collect(Collectors.toList())
        ));
        Button refresh = new Button("刷新状态");
        refresh.setOnAction(e -> {
            list.getItems().setAll(
                    downloadTasks.stream()
                            .map(t -> t.status.get() + " - " + t.target.getFileName())
                            .collect(Collectors.toList())
            );
        });
        VBox vbox = new VBox(refresh, list);
        vbox.setPadding(new Insets(10));
        s.setScene(new Scene(vbox, 400, 400));
        s.show();
    }

    private void toggleTheme() {
        isDarkMode = !isDarkMode;
        applyTheme(primaryStage.getScene());
        updateBookmarkBar(bookmarkBar);
    }

    private void applyTheme(Scene scene) {
        if (isDarkMode) {
            scene.getRoot().setStyle("-fx-base: #1e1e1e; -fx-background: #121212; -fx-control-inner-background: #2d2d2d; -fx-text-fill: white;");
        } else {
            scene.getRoot().setStyle("-fx-base: #ffffff; -fx-background: #ffffff; -fx-control-inner-background: #ffffff; -fx-text-fill: black;");
        }
    }

    private void saveBookmarks() {
        try {
            StringBuilder sb = new StringBuilder("[\n");
            for (int i = 0; i < bookmarks.size(); i++) {
                Map<String, String> bm = bookmarks.get(i);
                sb.append(String.format("  {\"title\":\"%s\",\"url\":\"%s\"}%s\n",
                        escapeJson(bm.get("title")),
                        escapeJson(bm.get("url")),
                        i == bookmarks.size() - 1 ? "" : ","
                ));
            }
            sb.append("]");
            Files.writeString(bookmarksFile, sb.toString(), StandardCharsets.UTF_8);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void loadBookmarks() {
        if (!Files.exists(bookmarksFile)) return;
        try {
            String content = Files.readString(bookmarksFile, StandardCharsets.UTF_8);
            String[] entries = content.replaceFirst("^\\[", "").replaceFirst("\\]$", "").split("\\},\\{");
            for (String entry : entries) {
                entry = entry.replace("{", "").replace("}", "");
                String[] pairs = entry.split("\",\"");
                String title = "", url = "";
                for (String pair : pairs) {
                    String[] kv = pair.split("\":\"", 2);
                    if (kv.length == 2) {
                        String key = kv[0].replace("\"", "");
                        String value = kv[1].replace("\"", "");
                        if ("title".equals(key)) title = value;
                        else if ("url".equals(key)) url = value;
                    }
                }
                if (!title.isEmpty() && !url.isEmpty()) {
                    bookmarks.add(Map.of("title", title, "url", url));
                }
            }
        } catch (Exception e) {
            /* ignore */
        }
    }

    private void saveHistory() {
        try {
            StringBuilder sb = new StringBuilder("[\n");
            for (int i = 0; i < historyList.size(); i++) {
                Map<String, String> h = historyList.get(i);
                sb.append(String.format("  {\"title\":\"%s\",\"url\":\"%s\",\"time\":\"%s\"}%s\n",
                        escapeJson(h.get("title")),
                        escapeJson(h.get("url")),
                        h.get("time"),
                        i == historyList.size() - 1 ? "" : ","
                ));
            }
            sb.append("]");
            Files.writeString(historyFile, sb.toString(), StandardCharsets.UTF_8);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void loadHistory() {
        if (!Files.exists(historyFile)) return;
        try {
            String content = Files.readString(historyFile, StandardCharsets.UTF_8);
            String[] entries = content.replaceFirst("^\\[", "").replaceFirst("\\]$", "").split("\\},\\{");
            for (String entry : entries) {
                entry = entry.replace("{", "").replace("}", "");
                String[] pairs = entry.split("\",\"");
                String title = "", url = "", time = "";
                for (String pair : pairs) {
                    String[] kv = pair.split("\":\"", 2);
                    if (kv.length == 2) {
                        String key = kv[0].replace("\"", "");
                        String value = kv[1].replace("\"", "");
                        if ("title".equals(key)) title = value;
                        else if ("url".equals(key)) url = value;
                        else if ("time".equals(key)) time = value;
                    }
                }
                if (!title.isEmpty() && !url.isEmpty() && !time.isEmpty()) {
                    historyList.add(Map.of("title", title, "url", url, "time", time));
                }
            }
        } catch (Exception e) {
            /* ignore */
        }
    }

    private String escapeJson(String s) {
        return s.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    private void showAlert(String title, String msg) {
        Alert alert = new Alert(Alert.AlertType.INFORMATION);
        alert.setTitle(title);
        alert.setHeaderText(null);
        alert.setContentText(msg);
        alert.showAndWait();
    }

    private String readApiKey() {
        Path keyFile = dataDir.resolve("api_key");
        if (Files.exists(keyFile)) {
            try {
                return Files.readString(keyFile, StandardCharsets.UTF_8).trim();
            } catch (Exception e) {
                System.err.println("读取 API Key 失败: " + e.getMessage());
            }
        }
        return null;
    }

    private CompletableFuture<String> callQwenAPI(String prompt) {
        String apiKey = readApiKey();
        if (apiKey == null || apiKey.isEmpty()) {
            return CompletableFuture.completedFuture(
                    "❌ 哎呀，我好像找不到你的魔法钥匙（API Key）！\n请在 ~/.lantern/api_key 文件里放好你的 DashScope Key，这样我才能帮你哦~"
            );
        }
        return CompletableFuture.supplyAsync(() -> {
            try {
                URL url = new URL("https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation");
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");
                conn.setRequestProperty("Authorization", "Bearer " + apiKey);
                conn.setRequestProperty("Content-Type", "application/json; charset=utf-8");
                conn.setDoOutput(true);

                String jsonInput = String.format("""
                        {
                          "model": "qwen-max",
                          "input": {
                            "messages": [
                              {"role": "user", "content": "%s"}
                            ]
                          },
                          "parameters": {
                            "result_format": "message"
                          }
                        }
                        """, prompt.replace("\"", "\\\""));

                try (OutputStream os = conn.getOutputStream()) {
                    byte[] input = jsonInput.getBytes(StandardCharsets.UTF_8);
                    os.write(input, 0, input.length);
                }

                int status = conn.getResponseCode();
                InputStream is = (status >= 200 && status < 300) ? conn.getInputStream() : conn.getErrorStream();
                String response = new String(is.readAllBytes(), StandardCharsets.UTF_8);

                if (status >= 200 && status < 300) {
                    int start = response.indexOf("\"content\":\"") + 11;
                    int end = response.indexOf("\"", start);
                    if (end > start) {
                        return response.substring(start, end).replace("\\n", "\n").replace("\\\"", "\"");
                    }
                    return "✅ 我收到了服务器的回复，但里面的内容有点乱，没能完全看懂:\n" + response;
                } else {
                    return "❌ 服务器好像不太开心 (" + status + "):\n" + response;
                }
            } catch (Exception e) {
                return "💥 哎呀，网络出问题了: " + e.getMessage();
            }
        });
    }

    class DownloadTask {
        String url;
        Path target;
        SimpleStringProperty status = new SimpleStringProperty("排队中...");
        long totalSize = 0;
        long downloaded = 0;
        boolean paused = false;
        List<CompletableFuture<Void>> chunks = new ArrayList<>();

        DownloadTask(String url, Path target) {
            this.url = url;
            this.target = target;
        }

        void start() {
            CompletableFuture.runAsync(() -> {
                try {
                    URL u = new URL(url);
                    HttpURLConnection conn = (HttpURLConnection) u.openConnection();
                    conn.setRequestMethod("HEAD");
                    totalSize = conn.getContentLengthLong();
                    conn.disconnect();

                    if (totalSize <= 0) {
                        singleThreadDownload();
                        return;
                    }

                    Files.createDirectories(target.getParent());
                    Files.deleteIfExists(target);
                    Files.createFile(target);

                    int numThreads = 4;
                    long chunkSize = totalSize / numThreads;
                    for (int i = 0; i < numThreads; i++) {
                        long start = i * chunkSize;
                        long end = (i == numThreads - 1) ? totalSize - 1 : start + chunkSize - 1;
                        int id = i;
                        chunks.add(CompletableFuture.runAsync(() -> downloadChunk(id, start, end)));
                    }

                    CompletableFuture.allOf(chunks.toArray(new CompletableFuture[0]))
                            .thenRun(() -> Platform.runLater(() -> status.set("搞定啦！")));
                } catch (Exception ex) {
                    Platform.runLater(() -> status.set("失败了: " + ex.getMessage()));
                }
            });
        }

        private void downloadChunk(int id, long start, long end) {
            try {
                URL u = new URL(url);
                HttpURLConnection conn = (HttpURLConnection) u.openConnection();
                conn.setRequestProperty("Range", "bytes=" + start + "-" + end);
                try (InputStream in = conn.getInputStream(); RandomAccessFile raf = new RandomAccessFile(target.toFile(), "rw")) {
                    raf.seek(start);
                    byte[] buffer = new byte[8192];
                    int bytesRead;
                    while (!paused && (bytesRead = in.read(buffer)) != -1) {
                        raf.write(buffer, 0, bytesRead);
                        downloaded += bytesRead;
                        Platform.runLater(() -> status.set(String.format("已下载 %.1f MB / %.1f MB (%.1f%%)",
                                downloaded / 1e6, totalSize / 1e6, 100.0 * downloaded / totalSize)));
                    }
                }
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }

        private void singleThreadDownload() {
            try {
                URL u = new URL(url);
                try (InputStream in = u.openStream()) {
                    Files.copy(in, target, StandardCopyOption.REPLACE_EXISTING);
                }
                Platform.runLater(() -> status.set("搞定啦！"));
            } catch (Exception ex) {
                Platform.runLater(() -> status.set("失败了: " + ex.getMessage()));
            }
        }

        void pause() {
            paused = true;
        }

        void resume() {
            if (paused) start();
        }
    }

    public abstract class LanternExtensionAPI {
        public abstract void log(String msg);
        public abstract void alert(String msg);
        public abstract String getBrowserVersion();
    }

    public static void main(String[] args) {
        launch(args);
    }
}