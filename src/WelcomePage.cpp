// SPDX-License-Identifier: LGPL-3.0-or-later
#include "WelcomePage.h"
#include "Settings.h"
#include <QByteArray>

namespace {
// 新标签页：搜索框 + 快捷链接 + 快捷键提示；通过 hash 传参（#engine=bing&dark=1）
const char *HTML = R"HTML(<html><head><meta charset='UTF-8'>
<style>
:root{
--bg1:#ff7a59;--bg2:#ff4d4d;--card:#ffffff;--text:#ffffff;--muted:rgba(255,255,255,.85);
--box-bg:rgba(255,255,255,.18);--box-border:rgba(255,255,255,.35);--hover:rgba(255,255,255,.28);}
body.dark{
--bg1:#1a2333;--bg2:#2d3a52;--card:#3a4a68;--text:#e8ecf4;--muted:#b9c2d4;
--box-bg:rgba(255,255,255,.07);--box-border:rgba(255,255,255,.16);--hover:rgba(255,255,255,.13);}
*{box-sizing:border-box}
body{margin:0;background:linear-gradient(135deg,var(--bg1),var(--bg2));color:var(--text);
font-family:'Noto Sans CJK SC','PingFang SC',sans-serif;display:flex;flex-direction:column;
justify-content:center;align-items:center;height:100vh;text-align:center;transition:background .4s}
.lantern{font-size:5.5em;margin-bottom:8px;animation:glow 2s infinite alternate;line-height:1}
h1{font-size:2.1em;margin:0 0 6px 0;font-weight:600}
.greet{color:var(--muted);font-size:1em;margin:0 0 26px 0}
.search{display:flex;width:min(620px,88vw);background:var(--box-bg);border:1px solid var(--box-border);
border-radius:26px;padding:4px;backdrop-filter:blur(6px);transition:border-color .2s,box-shadow .2s}
.search:focus-within{border-color:#fff;box-shadow:0 0 0 3px rgba(255,255,255,.18)}
.search input{flex:1;border:none;outline:none;background:transparent;color:var(--text);
font-size:1.05em;padding:10px 16px}
.search input::placeholder{color:var(--muted)}
.search button{border:none;border-radius:22px;padding:0 22px;font-size:1em;cursor:pointer;
background:#ffffff22;color:var(--text);font-weight:600}
.search button:hover{background:#ffffff3d}
.links{display:flex;flex-wrap:wrap;justify-content:center;gap:12px;margin-top:34px;max-width:640px}
.link{display:flex;flex-direction:column;align-items:center;gap:6px;width:86px;padding:14px 6px 10px;
background:var(--card);border-radius:14px;text-decoration:none;color:var(--text);
box-shadow:0 2px 8px rgba(0,0,0,.12);transition:transform .15s,box-shadow .15s}
.link:hover{transform:translateY(-4px);box-shadow:0 8px 20px rgba(0,0,0,.22)}
.link .ico{font-size:2em}
.link .nm{font-size:.8em;opacity:.9}
.hint{margin-top:38px;font-size:.82em;opacity:.8;line-height:2.1;max-width:760px}
kbd{background:var(--box-bg);border:1px solid var(--box-border);border-radius:5px;padding:1px 7px;font-size:.92em}
@keyframes glow{from{text-shadow:0 0 10px #fff,0 0 20px #ffb199}
to{text-shadow:0 0 22px #fff,0 0 44px #ff6b6b}}
</style></head><body>
<script>
(function(){
  var h={};location.hash.slice(1).split('&').forEach(function(p){var kv=p.split('=');if(kv[0])h[kv[0]]=kv[1]||'';});
  if(h.dark==='1')document.body.classList.add('dark');
  var hour=new Date().getHours();
  var greet=hour<6?'夜深了':hour<9?'早上好':hour<12?'上午好':hour<14?'中午好':hour<18?'下午好':'晚上好';
  document.getElementById('greet').textContent=greet+'，欢迎使用 Lantern！';
  var engine=h.engine||'bing';
  var bases={bing:'https://www.bing.com/search?q=',baidu:'https://www.baidu.com/s?wd=',google:'https://www.google.com/search?q='};
  var url=document.getElementById('searchInput');
  document.getElementById('searchForm').addEventListener('submit',function(e){
    e.preventDefault();var q=url.value.trim();if(!q)return;
    location.href=(bases[engine]||bases.bing)+encodeURIComponent(q);
  });
})();
</script>
<div class='lantern'>🏮</div>
<h1 id='greet'>欢迎使用 Lantern！</h1>
<p class='greet'>你的贴心浏览伙伴 · Qt WebEngine（Chromium 内核）</p>
<form class='search' id='searchForm'>
<input id='searchInput' type='text' placeholder='搜索，或直接输入网址后回车…' autofocus>
<button type='submit'>搜索</button>
</form>
<div class='links'>
<a class='link' href='https://www.baidu.com'><span class='ico'>🔍</span><span class='nm'>百度</span></a>
<a class='link' href='https://www.bing.com'><span class='ico'>🔎</span><span class='nm'>必应</span></a>
<a class='link' href='https://www.bilibili.com'><span class='ico'>📺</span><span class='nm'>哔哩哔哩</span></a>
<a class='link' href='https://www.zhihu.com'><span class='ico'>💡</span><span class='nm'>知乎</span></a>
<a class='link' href='https://weibo.com'><span class='ico'>🐦</span><span class='nm'>微博</span></a>
<a class='link' href='https://github.com'><span class='ico'>🐙</span><span class='nm'>GitHub</span></a>
<a class='link' href='https://mail.google.com'><span class='ico'>📧</span><span class='nm'>Gmail</span></a>
<a class='link' href='https://www.youtube.com'><span class='ico'>▶️</span><span class='nm'>YouTube</span></a>
</div>
<div class='hint'>
<kbd>Ctrl+T</kbd> 新标签 · <kbd>Ctrl+W</kbd> 关闭 · <kbd>Ctrl+Shift+N</kbd> 无痕 · <kbd>Ctrl+D</kbd> 收藏 ·
<kbd>Ctrl+F</kbd> 查找 · <kbd>Ctrl+H</kbd> 历史 · <kbd>Ctrl+J</kbd> 下载 · <kbd>Ctrl+Shift+S</kbd> 截图 ·
<kbd>F12</kbd> 开发者工具 · <kbd>F11</kbd> 全屏<br/>
在地址栏输入 <kbd>/ai 你的问题</kbd> 即可召唤 AI 助手～
</div>
</body></html>)HTML";
}

QString WelcomePage::html() { return QString::fromUtf8(HTML); }

QString WelcomePage::url() {
    // 把当前设置（搜索引擎 + 主题）通过 hash 传给页面
    const QString engine = Settings::instance().searchEngine();
    const bool dark = Settings::instance().darkTheme();
    const QString hash = QString("#engine=%1&dark=%2").arg(engine, dark ? "1" : "0");
    return "data:text/html;base64," + QString::fromLatin1(html().toUtf8().toBase64()) + hash;
}
