#pragma once

// Self-contained offline UI; no CDN, fonts or internet connection required.
static const char WEB_PAGE[] = R"raw(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#f7f7f7">
<title>我的魔杖 · Muggles' Wand</title>
<style>
:root{--bg:#f5f3ef;--paper:#fffdfa;--ink:#25231e;--muted:#625c51;--line:#e5e0d7;--control:#b9b1a4;--accent:#795613;--gold:#dbb76a;--soft:#eeebe4;--red:#922f27;--font:system-ui,-apple-system,"Segoe UI","Microsoft YaHei",sans-serif}
*{box-sizing:border-box}[hidden]{display:none!important}
body{margin:0;background:var(--bg);color:var(--ink);font:14px/1.55 var(--font);font-variant-numeric:tabular-nums}
button,input,select{font:inherit;color:inherit}
button,select,summary{cursor:pointer}
button{min-height:44px;padding:8px 14px;border:1px solid var(--control);border-radius:8px;background:var(--paper);font-weight:600}
button:hover:not(:disabled){border-color:var(--accent);background:var(--soft)}
button:disabled{background:var(--soft);color:var(--muted);cursor:not-allowed}
:focus-visible{outline:2px solid var(--accent);outline-offset:3px}
.primary{background:var(--gold);border-color:var(--gold);color:#2d230f}
.primary:hover:not(:disabled){background:#e7c783}
a{color:var(--accent);text-underline-offset:3px}
.danger{color:var(--red)}
header,main{max-width:932px;margin:auto;padding-inline:24px}
header{display:flex;justify-content:space-between;align-items:center;gap:12px;min-height:62px}
main{padding-bottom:32px}
.brand,.connection,.compact-title{display:flex;align-items:center;gap:8px}
.brand strong{font-size:16px}.brand-mark,.glyph{display:flex;color:var(--accent)}
.brand-mark svg{width:24px;height:24px}.glyph svg{width:20px;height:20px}
.connection{font-size:12px;color:var(--muted);text-align:right}
.dot{width:6px;height:6px;flex:none;border-radius:50%;background:var(--control)}.online .dot{background:var(--accent)}
.page-nav{display:flex;gap:24px;border-bottom:1px solid var(--line)}
.page-nav a{min-height:44px;display:flex;align-items:center;border-bottom:2px solid transparent;text-decoration:none;color:var(--muted);white-space:nowrap}
.page-nav a[aria-current]{color:var(--ink);font-weight:700;border-color:var(--ink)}
.mode-bar{display:flex;justify-content:space-between;align-items:center;gap:8px;margin:8px 0 0;color:var(--muted);font-size:12px}
.mode-bar strong{font-weight:500}.mode-bar button{font-size:12px}
.intro{margin:22px 0 16px}.intro h1{font-size:26px;font-weight:650;line-height:1.25;margin:0}
.intro h1:focus{outline:none}.back-link{display:inline-flex;align-items:center;min-height:44px;font-size:13px;text-decoration:none}
h2{font-size:17px;line-height:1.4;margin:0}h3{font-size:15px;margin:0 0 6px}
p{margin:6px 0;color:var(--muted)}.small{font-size:12px}.row{display:flex;align-items:center;flex-wrap:wrap;gap:8px}
.panel,.command-board,.device-links,.device-info{padding:16px;background:var(--paper);border:1px solid var(--line);border-radius:12px}
.panel+.panel,.panel+.device-links,.device-info+.device-links{margin-top:14px}
.panel>.row{margin-top:14px}
.command-board,.device-links,.device-info{padding:0 16px}
.command-row,.setting-row,.device-info>div,.device-links a{display:grid;grid-template-columns:minmax(0,1fr) auto;align-items:center;gap:8px 16px;padding:12px 0;border-bottom:1px solid var(--line)}
.command-row:last-child,.setting-row:last-child,.device-info>div:last-child,.device-links a:last-child{border:0}
.command-row .command-detail,.command-row>p,.command-row details{grid-column:1/-1;margin:0}
.command-row details{padding-top:0;border:0}.command-row summary{min-height:44px;padding-block:10px}
.command-row .command-detail{justify-self:end}.command-row[data-active="true"] h2,.command-actions button[data-active="true"]{color:var(--accent)}
.command-row a{display:inline-flex;align-items:center;min-height:44px}.command-actions{justify-content:flex-end}.command-row h2{overflow-wrap:anywhere}
.device-links a{color:var(--ink);min-height:54px;text-decoration:none}.device-links a:hover strong,.device-links a:hover .chevron{color:var(--accent)}
.device-links strong{display:block}.device-links small{display:block;margin-top:2px;color:var(--muted);font-size:12px}
.device-info{margin:0}.device-info>div{min-height:45px}.device-info dt{color:var(--muted)}
.device-info dd{margin:0;text-align:right;font-weight:600;overflow-wrap:anywhere}.device-info a{margin-left:6px}
.secondary-links{display:flex;gap:20px;margin-top:10px}.secondary-links a{display:flex;align-items:center;min-height:44px}
details{border-bottom:1px solid var(--line)}summary{padding:12px 0;min-height:44px;font-weight:550}
details>div{padding:0 0 12px}ol{padding-left:22px;color:var(--muted)}li{margin:6px 0}
label{display:block;font-weight:550;margin:12px 0 5px}
select,input{max-width:100%;min-height:44px;border:1px solid var(--control);border-radius:8px;background:var(--paper);padding:8px 10px;accent-color:var(--accent)}
select,.ha-form input:not([type=checkbox]){width:100%}
#led-count{width:auto;min-width:90px}
input[type=file]{width:100%;overflow:hidden}
.inline-label{display:flex;align-items:center;gap:8px;min-height:44px;margin:0}
.inline-label input{min-height:auto;width:18px;height:18px}
.ha-form fieldset{border:0;border-top:1px solid var(--line);padding:10px 0 0;margin:14px 0 0;min-width:0}
.ha-form fieldset h2:not(:first-child){margin-top:18px}
.notice{padding:10px 12px;margin:12px 0;background:var(--soft);border-radius:8px;color:var(--ink)}
.steps{display:grid;gap:12px}.step{border-bottom:1px solid var(--line);padding:8px 0}
.practice-result{padding:18px;margin:14px 0;background:var(--soft);border-radius:8px}
.practice-result strong{display:block;font-size:24px;font-weight:650;overflow-wrap:anywhere}
.practice-targets{display:flex;gap:6px;flex-wrap:wrap}.practice-targets button{min-width:44px}
.practice-targets [aria-pressed=true]{background:#f0e2c1;border-color:var(--accent)}
.binding-model{font-size:13px;margin:0 0 6px}
.binding-row{display:grid;grid-template-columns:minmax(60px,1fr) minmax(0,2fr) minmax(0,2fr) auto;gap:10px;align-items:end;padding:10px 0;border-bottom:1px solid var(--line)}
.binding-gesture{align-self:center;font-size:18px;overflow-wrap:anywhere}
.binding-field label{margin:0 0 3px;color:var(--muted);font-size:12px}
.binding-summary{display:none}.binding-preview-feedback{grid-column:1/-1;margin:0}
.binding-actions{background:var(--paper);padding:10px 0 4px}.binding-actions:has(#save-bindings:not([hidden])){position:sticky;bottom:0}.binding-actions p{flex-basis:100%;margin:0}
#ir-signals{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,280px),1fr));gap:10px;margin:12px 0}
.ir-slot{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:6px;padding:10px;border:1px solid var(--line);border-radius:10px}
.ir-slot input{grid-column:1/-1;min-width:0;width:100%;box-sizing:border-box}.ir-slot button{padding:7px 3px;font-size:13px;min-width:0}
progress{display:block;width:100%;height:12px;accent-color:var(--accent);margin-top:12px}
.section-title{display:flex;justify-content:space-between;align-items:center;gap:8px;margin:12px 0}
.log{height:180px;overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere;background:#272620;color:#f3efe4;border-radius:8px;padding:12px;font:12px/1.5 monospace}
.chart{display:block;width:100%;height:160px;background:var(--bg);border-radius:8px;margin:8px 0 12px}
.toast{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);width:max-content;max-width:calc(100% - 32px);background:#292823;color:#fff;padding:12px 16px;border-radius:8px;z-index:10}
dialog{border:1px solid var(--line);border-radius:12px;padding:20px;max-width:460px;width:calc(100% - 32px);color:var(--ink);background:var(--paper)}
dialog::backdrop{background:#29282377}dialog .row{justify-content:flex-end;margin-top:16px}
@media(max-width:600px){
 header,main{padding-inline:16px}.intro h1{font-size:22px}.intro{margin-top:14px}
 .page-nav{gap:20px}.command-row{grid-template-columns:1fr}.command-actions{justify-content:flex-start}.command-row .command-detail{justify-self:start}
 .command-row:first-child,.command-row:last-child,.setting-row{grid-template-columns:minmax(0,1fr) auto}
 .binding-row{grid-template-columns:minmax(0,1fr) auto;padding:5px 0}
 .binding-gesture{display:none}.binding-summary{grid-column:1/-1;display:flex;align-items:center;justify-content:space-between;gap:8px;text-align:left;border:0;padding:6px 0;background:transparent}
 .binding-summary span{font-size:13px;color:var(--muted);font-weight:400;overflow-wrap:anywhere}
 .binding-row:not([data-editing]) .binding-field,.binding-row:not([data-editing]) .binding-preview,.binding-row:not([data-editing]) .binding-preview-feedback{display:none}
 .binding-field{grid-column:1/-1}.binding-preview{justify-self:start}
}
</style>
</head>
<body>

<header><div class="brand"><span class="brand-mark"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="m4 20 12-12 3 3L7 23M15 3V1m5 5h3m-3-4 1-1M6 7H3m4-3V1"/><path d="m14 10 3 3"/></svg></span><strong>我的魔杖</strong></div><div id="connection" class="connection" role="status"><span class="dot"></span><span id="connection-text">正在连接魔杖…</span></div></header>
<main>
<div class="notice" id="offline" role="status" hidden>连接魔杖 Wi-Fi 后重试。休眠时短按底部按键唤醒。</div>
<nav class="page-nav" aria-label="后台导航">
<a href="#control" aria-current="page">发送指令</a>
<a href="#settings">配置</a>
<a href="#device">设备</a>
</nav>
<div class="mode-bar" id="mode-bar"><div><strong id="mode">等待设备状态</strong></div><button id="back-mode" hidden data-device data-cmd="back" data-message="正在退出当前模式，恢复挥杖识别">返回手势识别</button></div>
<section id="control" data-page aria-labelledby="control-title">
<div class="intro"><h1 id="control-title" tabindex="-1">发送指令</h1></div>
<div class="command-board">
<article class="command-row"><div class="compact-title"><span class="glyph"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M9 18h6m-5 3h4M8 13a6 6 0 1 1 8 0c-1 1-1 2-1 2H9s0-1-1-2Z"/></svg></span><h2>照明魔法</h2></div><button class="primary" data-device data-cmd="gesture 5" data-message="照明开关指令已发送，请看看魔杖灯光">切换照明</button></article>
<article class="command-row" id="light-settings" data-mode="5"><h2>空中光绘</h2><button data-device data-cmd="gesture 7" data-message="正在进入光绘模式，挥动魔杖试试看">进入光绘</button><details><summary>选择图案</summary><div><label for="pattern">内置图案</label><select id="pattern" disabled><option value="">正在读取魔杖图案…</option></select><label class="inline-label"><input type="checkbox" id="reverse">反向播放（适合向上或向左挥动）</label><button data-device id="apply-pattern">应用图案</button><p class="small" id="pattern-feedback" role="status">图案只在本次开机期间生效。</p></div></details></article>
<article class="command-row" id="remote"><h2>蓝牙遥控</h2><div class="row command-actions"><button data-device data-mode="3" data-cmd="gesture 4" data-message="正在进入鼠标模式，请确认电脑已与魔杖配对">进入鼠标模式</button><button data-device data-mode="4" data-cmd="gesture 6" data-message="正在进入音量模式，转动魔杖调整音量">进入音量模式</button></div></article>
<article class="command-row"><h2>红外配件</h2><div class="row command-actions"><a href="#ir-settings" id="ir-command-link">去配置</a><select id="ir-command-slot" aria-label="选择红外信号" hidden></select><button id="ir-command-send" data-device hidden disabled>重放信号</button></div><p class="small" id="ir-command-state" role="status"></p></article>
<article class="command-row"><h2>Home Assistant</h2><div class="row"><a href="#ha-settings" id="ha-command-link">去配置</a><button id="ha-test" data-device disabled hidden>发送测试事件</button></div><p id="ha-test-result" class="small" role="status" hidden></p></article>
</div>
<div class="secondary-links"><a href="#learn">挥杖练习</a><a href="#help">使用帮助</a></div>
</section>
<section id="settings" data-page aria-labelledby="settings-title" hidden>
<div class="intro"><h1 id="settings-title" tabindex="-1">配置</h1></div>
<div class="device-links">
<a href="#gesture-settings"><span><strong>手势与灯效</strong><small id="settings-gestures">等待读取</small></span><span class="chevron" aria-hidden="true">›</span></a>
<a href="#model-settings"><span><strong>更新手势模型</strong><small id="settings-model">等待读取</small></span><span class="chevron" aria-hidden="true">›</span></a>
<a href="#ha-settings"><span><strong>Home Assistant</strong><small id="settings-ha">等待读取</small></span><span class="chevron" aria-hidden="true">›</span></a>
<a href="#ir-settings"><span><strong>红外配件</strong><small id="settings-ir">等待读取</small></span><span class="chevron" aria-hidden="true">›</span></a>
</div>
<section class="panel"><div class="setting-row"><label for="led-count">灯珠数量</label><div class="row"><select id="led-count" disabled><option>35</option><option>36</option><option>37</option><option>38</option><option selected>39</option><option>40</option><option>41</option><option>42</option><option>43</option><option>44</option><option>45</option></select><button class="primary" id="led-save" data-device disabled>保存</button></div></div><p id="led-feedback" role="status"></p></section>
</section>
<section id="ha-settings" data-page data-parent="settings" aria-labelledby="ha-settings-title" hidden>
<div class="intro"><a class="back-link" href="#settings">返回配置</a><h1 id="ha-settings-title" tabindex="-1">Home Assistant</h1></div>
<form id="ha-form" class="panel ha-form" autocomplete="off">
<label class="inline-label"><input id="ha-enabled" type="checkbox">启用 Home Assistant</label>
<fieldset id="ha-fields" hidden disabled>
<h2>家庭 Wi-Fi</h2>
<label for="ha-ssid">家庭 Wi-Fi 名称（2.4 GHz）</label><input id="ha-ssid" maxlength="32" required>
<label for="ha-wifi-key">Wi-Fi 密码</label><input id="ha-wifi-key" type="password" maxlength="63" autocomplete="new-password">
<h2>MQTT</h2>
<label for="ha-broker">MQTT 服务器地址</label><input id="ha-broker" maxlength="63" placeholder="例如 192.168.1.20" required>
<label for="ha-port">端口</label><input id="ha-port" type="number" min="1" max="65535" value="1883" required>
<label for="ha-user">MQTT 用户名</label><input id="ha-user" maxlength="31">
<label for="ha-key">MQTT 密码</label><input id="ha-key" type="password" maxlength="63" autocomplete="new-password">
</fieldset>
<p id="ha-config-state" class="small">未启用</p>
<div class="row" id="ha-actions" hidden><button class="primary" id="ha-save" data-device disabled>保存配置</button><button type="button" class="primary" id="ha-reboot" data-device data-action="reboot" hidden>重启生效</button></div>
<p id="ha-save-result" role="status"></p>
</form>
</section>
<section id="gesture-settings" data-page data-parent="settings" aria-labelledby="gesture-settings-title" hidden>
<div class="intro"><a class="back-link" href="#settings">返回配置</a><h1 id="gesture-settings-title" tabindex="-1">手势设置</h1><p>为每个手势选择功能和灯效。</p></div>
<section class="panel"><p id="binding-model" class="binding-model">正在读取魔杖的模型…</p><div id="binding-profile-note" class="notice">连接魔杖后显示当前模型支持的手势。</div><div id="gesture-mapping"></div><div class="row binding-actions"><button class="primary" data-device id="save-bindings" disabled>保存到魔杖</button><button data-device id="reload-bindings" disabled>重新读取 / 撤销修改</button><p id="binding-feedback" role="status" aria-live="polite">正在读取配置…</p></div><details><summary>功能与灯效说明</summary><div><p>附加灯效短暂播放，照明、鼠标及光绘本身的灯光仍会保留。选择“不执行功能”也能单独播放灯效。预览不会执行功能。</p><p>照明和光绘使用自带灯带；鼠标和音量需要蓝牙配对；红外信号需要先在「红外配件」中学习和连接。多个手势可以执行同一功能，也可以选择“不执行功能”。</p><p>鼠标、音量和光绘会占用动作传感器；单击底部按键或点击「返回手势识别」后，再使用其他手势。</p></div></details></section>
</section>
<section id="ir-settings" data-page data-parent="settings" aria-labelledby="ir-settings-title" hidden>
<div class="intro"><a class="back-link" href="#settings">返回配置</a><h1 id="ir-settings-title" tabindex="-1">红外配件</h1></div>
<section class="panel"><div class="setting-row"><div><h2>连接 ESP32-C3</h2><p id="ir-link-state" class="small">正在搜索</p></div><div class="row"><button id="ir-pair" data-device hidden>连接</button><button id="ir-unpair" data-device hidden>断开</button></div></div><div class="row"><button id="ir-radio-toggle" data-device data-cmd="espnow">开启无线通信</button><button id="ir-scan" data-device>搜索设备</button></div></section>
<section class="panel"><div class="setting-row"><h2>已学信号</h2><button id="ir-add" data-device>添加信号</button></div><p id="ir-feedback" role="status"></p><div id="ir-signals"></div></section>
</section>
<section id="learn" data-page data-parent="control" aria-labelledby="learn-title" hidden>
<div class="intro"><a class="back-link" href="#control">返回指令页</a><h1 id="learn-title" tabindex="-1">挥杖练习</h1></div>
<section class="panel practice"><h2 id="practice-target-label">选择手势</h2><div class="practice-targets" id="practice-targets" role="group" aria-labelledby="practice-target-label"></div><select id="practice-target" hidden></select><div class="practice-result" role="status" aria-live="polite"><strong id="practice-title">准备好了吗？</strong><p id="practice-detail">停稳 → 挥杖 → 停稳。识别成功会执行已绑定的动作。</p></div><div class="row"><button class="primary" data-device id="practice-start">开始练习</button><button id="practice-end" data-device hidden>结束练习</button></div></section><details><summary>第一次使用</summary><div class="steps"><article class="step"><h3>开机，连接专属 Wi-Fi</h3><p>单击魔杖底部按键开机。在手机或电脑中连接 <code id="wifi-name">Muggles' Wand</code>，密码 <code id="wifi-pass">12345678</code>。这个网络无需互联网。</p></article><article class="step"><h3>试一下照明</h3><p>回到<a href="#control">指令页</a>试试照明，再练习挥杖。</p></article><article class="step"><h3>先停稳，再一气呵成</h3><p><span id="guide-action">按当前模型对应的手势挥杖，识别后执行你保存的功能和灯效。</span></p></article></div></details>
</section>
<section id="device" data-page aria-labelledby="device-title" hidden>
<div class="intro"><h1 id="device-title" tabindex="-1">设备信息</h1></div>
<dl class="device-info panel">
<div><dt>当前模式</dt><dd id="device-mode">等待读取</dd></div>
<div><dt>固件版本</dt><dd id="version">—</dd></div>
<div><dt>蓝牙连接</dt><dd id="bluetooth">等待读取</dd></div>
<div><dt>充电状态</dt><dd id="charging">等待读取</dd></div>
<div><dt>红外配件</dt><dd><span id="receiver-status">等待读取</span><a href="#ir-settings" id="receiver-fix" hidden>查看</a></dd></div>
<div><dt>家庭 Wi-Fi</dt><dd id="ha-wifi-state">等待读取</dd></div>
<div><dt>Home Assistant</dt><dd id="ha-state">等待读取</dd></div>
</dl>
<div class="device-links">
<a href="#receiver">连接设置 <span aria-hidden="true">›</span></a>
<a href="#power">电源与 Wi-Fi <span aria-hidden="true">›</span></a>
<a href="#firmware">升级系统固件 <span aria-hidden="true">›</span></a>
<a href="#advanced">采集与诊断 <span aria-hidden="true">›</span></a>
</div>
</section>
<section id="power" data-page data-parent="device" aria-labelledby="power-title" hidden>
<div class="intro"><a class="back-link" href="#device">返回设备</a><h1 id="power-title" tabindex="-1">电源与 Wi-Fi</h1></div>
<div class="panel">
<div class="setting-row"><span>重新启动</span><button data-device data-action="reboot">重新启动</button></div>
<div class="setting-row"><span>暂时待机</span><button data-device data-action="sleep">进入休眠</button></div>
</div>
<div class="panel"><h2>断开设备</h2>
<div class="setting-row"><span>关闭电源</span><button data-device class="danger" data-action="shutdown">关闭魔杖</button></div>
<div class="setting-row"><span>断开网页连接</span><button data-device class="danger" data-action="ap">关闭魔杖 Wi-Fi</button></div>
</div>
</section>
<section id="receiver" data-page data-parent="device" aria-labelledby="receiver-title" hidden>
<div class="intro"><a class="back-link" href="#device">返回设备</a><h1 id="receiver-title" tabindex="-1">连接设置</h1></div>
<div class="panel">
<div class="setting-row"><div><h2>蓝牙</h2><p id="ble-setting-state" class="small">等待读取</p><p id="pair-help" class="small" hidden>在设备的蓝牙设置中选择 <strong class="ble-name">Muggles' Wand</strong>。</p></div><button data-device id="ble-toggle" data-cmd="ble" data-message="已请求切换蓝牙">切换蓝牙</button></div>
</div>
</section>
<section id="model-settings" data-page data-parent="settings" aria-labelledby="model-settings-title" hidden>
<div class="intro"><a class="back-link" href="#settings">返回配置</a><h1 id="model-settings-title" tabindex="-1">更新手势模型</h1><p>选择电脑训练工具导出的 .wandmodel 文件。</p></div>
<section class="panel"><h2>当前模型</h2><p id="model-info">正在读取当前模型</p><label for="model-file">选择模型文件</label><input id="model-file" type="file" accept=".wandmodel"><button class="primary" id="upload-model-file" data-device disabled hidden>上传模型</button><progress id="model-upload-progress" max="100" hidden></progress><p id="model-upload-message" role="status"></p><details><summary>模型和系统有什么区别？</summary><div><p>模型决定识别出了哪个手势。功能和灯效由魔杖保存，更新模型不包含功能配置。新增手势在“手势设置”中分配功能。</p><p>只有新增系统功能或修复程序时才需要升级系统固件。</p></div></details></section>
</section>
<section id="firmware" data-page data-parent="device" aria-labelledby="firmware-title" hidden>
<div class="intro"><a class="back-link" href="#device">返回设备</a><h1 id="firmware-title" tabindex="-1">升级系统固件</h1></div>
<section class="panel"><p>准备适配这根魔杖的应用固件 .bin，并保持供电。</p><button class="primary" data-device data-action="ota">进入固件升级</button> <a href="/update" id="ota-link" hidden>打开升级页</a></section>
</section>
<section id="advanced" data-page data-parent="device" aria-labelledby="advanced-title" hidden>
<div class="intro"><a class="back-link" href="#device">返回设备</a><h1 id="advanced-title" tabindex="-1">采集与诊断</h1></div>
<section class="panel"><p id="capture-state" role="status">等待读取设备状态</p><div class="row"><button class="primary" data-device id="capture-start" data-cmd="imu" data-message="正在切换到动作采集">采集动作数据</button><button data-device id="capture-stop" hidden data-cmd="back" data-message="正在结束采集，恢复手势识别">结束采集 / 恢复施法</button></div><details><summary>高级命令</summary><div><div class="row"><button data-device data-cmd="stop" data-message="正在暂停动作检测，恢复施法请点返回手势识别">暂停动作检测</button></div><div class="row"><button data-device data-cmd="info" data-message="已请求设备信息，请查看日志">读取设备信息</button><button data-device data-cmd="debug" data-message="已请求保持唤醒；恢复自动休眠需要重启魔杖">保持唤醒</button></div><p class="small">“保持唤醒”会增加耗电，重启后恢复自动休眠。</p></div></details></section><details id="diagnostic-log"><summary>原始日志</summary><div><div class="section-title"><h3>诊断日志</h3><button id="clear-log">清空日志</button></div><p class="small">这里保留原始信息，方便排查问题；不会上传到互联网。</p><pre class="log" id="log-window" aria-label="诊断日志"></pre></div></details><div id="capture-charts" hidden><p class="small">采集动作数据后显示实时曲线：红色为横向角速度，金色为纵向角速度（rad/s）。</p><canvas id="motion-chart" class="chart" aria-label="动作角速度曲线"></canvas><p class="small">动作检测阶段：0 初始化 / 1 静止 / 2 开始运动 / 3 有效挥动 / 4 完成。</p><canvas id="state-chart" class="chart" aria-label="手势检测阶段曲线"></canvas></div>
</section>
<section id="help" data-page data-parent="control" aria-labelledby="help-title" hidden>
<div class="intro"><a class="back-link" href="#control">返回指令页</a><h1 id="help-title" tabindex="-1">使用帮助</h1></div>
<div class="panel">
<details><summary>挥了没反应</summary><div><p>先切回手势识别，再停稳、挥杖、停稳。</p><a href="#learn">打开挥杖练习</a></div></details>
<details><summary>连不上魔杖热点</summary><div><p>短按底部按键唤醒，连接魔杖 Wi-Fi。手机提示无法上网时，选择继续连接。</p><a href="#control">回到指令页</a></div></details>
<details><summary>蓝牙还没配对</summary><div><p>在手机或电脑的蓝牙设置中选择魔杖。</p><a href="#receiver">查看蓝牙状态</a></div></details>
<details><summary>想用自己训练的手势</summary><div><p>在电脑端训练并导出 .wandmodel 文件。</p><a href="#model-settings">打开模型上传</a></div></details>
</div>
</section>

</main>
<div class="toast" id="toast" role="status" aria-live="polite" hidden></div>
<dialog id="confirm-dialog" aria-labelledby="confirm-title"><h2 id="confirm-title"></h2><p id="confirm-body"></p><div class="row"><button id="cancel-action">取消</button><button class="primary" id="confirm-action">继续</button></div></dialog>
<script>
'use strict';
const $=id=>document.getElementById(id);
const haStates=['未启用','等待 Wi-Fi','正在连接 MQTT','正在注册设备','已连接 MQTT','服务不可用'];
let haSaveTimer,haTestTimer;
function renderHa(s){
 const state=s.haState||0;
 $('ha-state').textContent=haStates[state]||'等待读取';
 $('ha-wifi-state').textContent=['未启用','正在连接','已连接','已连接','已连接','不可用'][state]||'等待读取';
 $('ha-test').hidden=state!==4;
 $('ha-command-link').hidden=state===4;
 $('ha-test').disabled=state!==4||!!haTestTimer;
 $('settings-ha').textContent=$('ha-config-state').textContent=haStates[state];
 $('ha-save').disabled=!!haSaveTimer;
 if(!$('ha-enabled').dataset.edited){$('ha-enabled').checked=state!==0;updateHaFields()}
}
function updateHaFields(){
 $('ha-fields').hidden=$('ha-fields').disabled=!$('ha-enabled').checked;
 $('ha-actions').hidden=!$('ha-enabled').checked&&!$('ha-enabled').dataset.edited;
}
function editHa(){
 $('ha-enabled').dataset.edited='true';
 $('ha-reboot').hidden=true;
 $('ha-save').hidden=false;
 $('ha-save-result').textContent='';
 updateHaFields();
}
$('ha-form').addEventListener('input',editHa);
$('ha-form').onsubmit=event=>{
 event.preventDefault();
 if(haSaveTimer||$('ha-save').disabled)return;
 const fields=['ha-ssid','ha-wifi-key','ha-broker','ha-port','ha-user','ha-key'].map(id=>$(id).value);
 if(send('web:ha-config\n'+[$('ha-enabled').checked?'1':'0',...fields].join('\n'))){
  $('ha-save').disabled=true;
  $('ha-save-result').textContent='正在保存…';
  haSaveTimer=setTimeout(()=>haReply({type:'haConfig',ok:false,message:'尚未收到保存确认，请重试。'}),6000);
 }
};
$('ha-test').onclick=()=>{
 if(haTestTimer||!send('web:ha-test'))return;
 $('ha-test').disabled=true;
 $('ha-test-result').hidden=false;
 $('ha-test-result').textContent='正在发送…';
 haTestTimer=setTimeout(()=>haReply({type:'haTest',ok:false}),6000);
};
function haReply(data){
 if(data.type==='haConfig'){
  clearTimeout(haSaveTimer);haSaveTimer=0;
  $('ha-save-result').textContent=data.ok?'已保存。重启魔杖后生效。':data.message;
  if(data.ok){
   $('ha-enabled').dataset.edited='true';
   $('ha-wifi-key').value=$('ha-key').value='';
   $('ha-save').hidden=true;
   $('ha-reboot').hidden=false;
  }
 }else{
  clearTimeout(haTestTimer);haTestTimer=0;
  $('ha-test-result').textContent=data.ok?'测试事件已发送，请到 Home Assistant 查看。':'测试未发送，请检查设备页的连接状态。';
 }
}
const names=[null,null,null,null,'隔空鼠标','照明魔法','音量旋钮','空中光绘','HA 场景 1','HA 场景 2','HA 场景 3','HA 场景 4'];
let irSlots=[],irPrevious=[];
function irCommand(type,slot){if(send('web:ir '+type+' '+slot)){const note=type===0x41?'正在等待遥控器，请短按一次…':type===0x40?'正在搜索…':'正在发送指令…';$('ir-feedback').textContent=note;if(type===0x42)$('ir-command-state').textContent=note}}
function renderIr(slots,link,conflict,radio){if(!Array.isArray(slots)||slots.length!==32)return;irSlots=slots;link=link||{};
 const paired=!!link.paired,ready=!!link.online&&radio&&!conflict;
 $('ir-radio-toggle').textContent=radio?'关闭无线通信':'开启无线通信';$('ir-scan').disabled=!radio;
 $('ir-pair').hidden=paired||!link.candidate;$('ir-pair').disabled=!link.writable||!radio||conflict;
 $('ir-pair').dataset.mac=link.candidate||'';
 $('ir-unpair').hidden=!paired;$('ir-unpair').disabled=!link.writable;
 $('ir-link-state').textContent=!radio?'无线通信已关闭':conflict?'Wi-Fi 信道冲突':paired?(ready?'已连接到 '+link.paired:'等待 '+link.paired):link.candidate?'发现 '+link.candidate:'正在搜索 C3';
 $('settings-ir').textContent=!radio?'已关闭':paired?(ready?'已连接':'等待设备'):link.candidate?'发现设备':'未连接';
 const active=document.activeElement,editing=active?.closest?.('.ir-slot');const editSlot=editing?.dataset.slot,editValue=editing?active.value:null,editStart=editing?active.selectionStart:null;
 $('ir-signals').replaceChildren();
 slots.forEach((slot,i)=>{
  const saved=[1,3,4].includes(slot.state);
  if(!saved&&slot.state!==2&&slot.state!==5&&slot.state!==6)return;
  const row=document.createElement('div');row.className='ir-slot';row.dataset.slot=i;
  const name=document.createElement('input');name.maxLength=16;name.value=slot.name||'红外信号 '+(i+1);name.setAttribute('aria-label','红外信号名称');
  const make=(label,fn,disabled=false)=>{const b=document.createElement('button');b.type='button';b.textContent=label;b.disabled=disabled;b.onclick=fn;return b};
  row.append(name,make('保存名称',()=>{const value=name.value.trim();if(value&&new TextEncoder().encode(value).length<=48)send('web:ir-name '+i+' '+value);else toast('名称不能为空或过长')},!ready||!saved),make('重新学习',()=>irCommand(0x41,i),!ready),make('重放',()=>irCommand(0x42,i),!ready||!saved),make('删除',()=>irCommand(0x43,i),!ready||!saved));
  $('ir-signals').append(row);
 });
 if(editSlot!==undefined){const input=[...$('ir-signals').querySelectorAll('.ir-slot')].find(row=>row.dataset.slot===editSlot)?.querySelector('input');if(input){input.value=editValue;input.focus();input.setSelectionRange(editStart,editStart)}}
 slots.forEach((slot,i)=>{const label=[1,3,4].includes(slot.state)?slot.name||'红外信号 '+(i+1):null;names[12+i]=label;document.querySelectorAll('.binding-action-field select').forEach(select=>{let option=[...select.options].find(o=>o.value===String(12+i));if(label&&!option){option=document.createElement('option');option.value=12+i;select.append(option)}if(option){if(label)option.textContent=label;else if(Number(select.value)!==12+i)option.remove();else option.textContent='已删除的红外信号'}})});
 $('ir-add').disabled=!ready||!slots.some(slot=>slot.state===0);
 const available=slots.flatMap((slot,i)=>ready&&[1,3,4].includes(slot.state)?[{id:i,name:slot.name||'红外信号 '+(i+1)}]:[]);
 const select=$('ir-command-slot'),previous=select.value;
 if(JSON.stringify(available)!==select.dataset.options){select.replaceChildren(...available.map(item=>{const option=document.createElement('option');option.value=item.id;option.textContent=item.name;return option}));select.dataset.options=JSON.stringify(available);if(available.some(item=>String(item.id)===previous))select.value=previous}
 select.hidden=$('ir-command-send').hidden=!available.length;$('ir-command-send').disabled=!available.length;
 $('ir-command-link').hidden=!!available.length;
 $('ir-command-state').textContent=!radio?'无线通信已关闭':conflict?'Wi-Fi 信道冲突':paired&&!ready?'等待 C3 上线':'';
 const changed=slots.findIndex((s,i)=>s.online&&s.state!==irPrevious[i]);
 if(changed>=0){const state=slots[changed].state;if(state===3)$('ir-feedback').textContent='学习成功，已保存到 C3';else if(state===4){$('ir-feedback').textContent='已重放一次';$('ir-command-state').textContent='已重放一次'}else if(state===5)$('ir-feedback').textContent='等待超时，请重试';else if(state===6)$('ir-feedback').textContent='操作失败，请检查 C3';else if(state===2)$('ir-feedback').textContent='正在等待遥控器，请短按一次';}
 else if(!ready)$('ir-feedback').textContent='';
 irPrevious=slots.map(s=>s.state);
}
const modeNames={'-1':'动作检测已暂停',1:'动作数据采集中',2:'挥杖识别已就绪',3:'隔空鼠标',4:'音量旋钮',5:'空中光绘'};
let ws,retryTimer,statusTimer,toastTimer,lastStatus=0,practice=false,practiceTimer,pending='',logs=[],confirmedAction=null,expectedDisconnect='';
let previewFeedback=null;
let ledDirty=false,ledSaving=false,ledCurrent=39;
let bindingState=null,bindingDraft=null,bindingEffectsDraft=null,bindingRevision=0,bindingDirty=false,bindingSaving=false,bindingTimer;
function toast(message){$('toast').textContent=message;$('toast').hidden=false;clearTimeout(toastTimer);toastTimer=setTimeout(()=>$('toast').hidden=true,4500)}
function online(connected){if(!connected){lastStatus=0;ledSaving=false;$('led-count').disabled=true;}$('connection').classList.toggle('online',connected);$('connection-text').textContent=connected?'已连接魔杖':expectedDisconnect||'连接已断开，正在重试';$('offline').hidden=connected;document.querySelectorAll('[data-device]').forEach(b=>b.disabled=!connected);$('pattern').disabled=!connected||!$('pattern').value;if(!connected){$('ir-add').disabled=true;$('mode-bar').hidden=true;$('device-mode').textContent='设备已离线';$('capture-state').textContent='设备已离线';$('capture-charts').hidden=true;$('ha-test').hidden=true;$('ha-command-link').hidden=false;$('pair-help').hidden=true;$('ha-wifi-state').textContent='等待读取';$('ha-state').textContent='等待读取';$('back-mode').hidden=true;document.querySelectorAll('[data-mode]').forEach(card=>card.dataset.active='false');$('mode').textContent='设备已离线';$('bluetooth').textContent='等待重新连接';$('charging').textContent='等待重新连接';$('receiver-status').textContent='等待读取';endPractice(false);bindingSaving=false;clearTimeout(bindingTimer);document.querySelectorAll('#gesture-mapping select').forEach(s=>s.disabled=true);document.querySelectorAll('#ir-signals button').forEach(b=>b.disabled=true);$('ir-command-send').disabled=true;$('save-bindings').disabled=true;$('reload-bindings').disabled=true}}
function send(command,message){if(!ws||ws.readyState!==WebSocket.OPEN||Date.now()-lastStatus>10000){toast('还没有连上魔杖，请检查 Wi-Fi 后重试');return false}ws.send(command);if(message)toast(message);return true}
$('led-count').onchange=()=>{ledDirty=Number($('led-count').value)!==ledCurrent;$('led-feedback').textContent='';$('led-save').disabled=!ledDirty||ledSaving};
$('led-save').onclick=()=>{if(ledSaving||!ledDirty||!send('web:led-count '+$('led-count').value))return;ledSaving=true;$('led-save').disabled=true;$('led-feedback').textContent='正在保存…'};
function renderLedCount(s){if(Number.isInteger(s.ledCount)&&s.ledCount>=35&&s.ledCount<=45){ledCurrent=s.ledCount;if(!ledDirty&&!ledSaving)$('led-count').value=String(s.ledCount)}$('led-count').disabled=!!s.modelUpdating;$('led-save').disabled=!!s.modelUpdating||!ledDirty||ledSaving}
function connect(){clearTimeout(retryTimer);ws=new WebSocket((location.protocol==='https:'?'wss://':'ws://')+location.host+'/ws');ws.onopen=()=>{lastStatus=0;pending='';ws.send('web:status');ws.send('web:ir 64 0');clearInterval(statusTimer);statusTimer=setInterval(()=>{if(ws.readyState!==WebSocket.OPEN)return;if(Date.now()-lastStatus>10000){ws.close();return}ws.send('web:status');ws.send('web:ir 64 0')},3000)};ws.onmessage=e=>{try{const data=JSON.parse(e.data);if(data.type==='busy'){toast(data.message);return}if(data.type==='ir'){if(!data.ok)toast('红外指令未发送，请检查红外配件');return}if(data.type==='irPair'){if(!data.ok)toast('连接设置失败，请重新搜索');return}if(data.type==='effect'){if(previewFeedback)previewFeedback.textContent=data.ok?'已发送灯效预览':data.message;return}if(data.type==='bindings'){bindingSaving=false;clearTimeout(bindingTimer);if(data.ok){bindingDirty=false;bindingDraft=null;bindingEffectsDraft=null} $('binding-feedback').textContent=data.message;toast(data.message);ws.send('web:status');return}if(data.type==='ledCount'){ledSaving=false;if(data.ok){ledCurrent=data.count;ledDirty=false;$('led-count').value=String(data.count)}$('led-feedback').textContent=data.ok?'已保存，立即生效':'保存失败，请重试';$('led-save').disabled=!ledDirty;ws.send('web:status');return}if(data.type==='recognition'){showRecognition(data);return}if(data.type==='haConfig'||data.type==='haTest'){haReply(data);return}if(data.type==='status'){if(!lastStatus&&!$('model-settings').hidden)readModelInfo();lastStatus=Date.now();expectedDisconnect='';online(true);renderStatus(data);return}}catch(_){}appendLog(String(e.data))};ws.onclose=()=>{clearInterval(statusTimer);online(false);retryTimer=setTimeout(connect,3000)};ws.onerror=()=>ws.close()}
function renderStatus(s){
 renderIr(s.irSlots,s.irLink,s.receiverConflict,s.receiver);
 renderHa(s);
 renderLedCount(s);
 const mode=modeNames[s.mode]||'正在切换玩法';
 $('device-mode').textContent=s.mode===2&&(s.imuReady===false||s.inferenceReady===false)?'动作检测恢复中':mode;
 $('mode-bar').hidden=activePage!=='control'&&(s.mode===2||(activePage==='advanced'&&s.mode===1));
 $('ble-setting-state').textContent=s.bleConnected?'已配对连接':s.bleEnabled?'等待配对':'已关闭';
 $('ble-toggle').textContent=s.bleEnabled?'关闭蓝牙':'开启蓝牙';
 $('pair-help').hidden=!s.bleEnabled||s.bleConnected;
 $('receiver-fix').hidden=!s.receiverConflict;
 $('settings-gestures').textContent=Array.isArray(s.gestureLabels)?s.gestureLabels.length+' 个手势':'等待读取';
 if(s.gestureProfileTitle) $('settings-model').textContent=s.gestureProfileTitle;
 const capturing=s.mode===1;
 $('capture-state').textContent=capturing?'正在采集动作，手势识别已暂停':'准备采集动作';
 $('capture-start').hidden=capturing;
 $('capture-stop').hidden=$('capture-charts').hidden=!capturing;
 if(capturing)scheduleCharts();$('back-mode').hidden=s.mode===2;renderBindings(s);syncPractice(s);$('upload-model-file').disabled=!!s.modelUpdating||modelUploading||!$('model-file').files.length;if(s.modelUpdating){document.querySelectorAll('[data-device]').forEach(b=>b.disabled=true);$('connection-text').textContent='正在更新模型'}$('mode').textContent=s.modelUpdating?'正在更新模型':s.mode===2&&(s.imuReady===false||s.inferenceReady===false)?'动作检测恢复中，请稍候':modeNames[s.mode]||'正在切换玩法';document.querySelectorAll('[data-mode]').forEach(card=>card.dataset.active=String(Number(card.dataset.mode)===s.mode));$('bluetooth').textContent=s.bleConnected?'已配对连接':s.bleEnabled?'已开启，等待配对':'蓝牙已关闭';$('charging').textContent=s.charging?'正在充电':'未在充电';$('version').textContent=s.version;$('wifi-name').textContent=s.ssid;$('wifi-pass').textContent=s.password;document.querySelectorAll('.ble-name').forEach(el=>el.textContent=s.bleName);$('receiver-status').textContent=s.receiverConflict?'信道冲突':s.irLink?.online?'已连接':s.irLink?.paired?'等待设备':'未连接';const select=$('pattern');const options=JSON.stringify(s.patterns);if(select.dataset.options!==options){const previous=select.value;select.replaceChildren();s.patterns.forEach((name,index)=>{const option=document.createElement('option');option.value=index;option.textContent=({MugglesWand:'魔杖文字',Star:'星星',Rainbow:'彩虹'})[name]||name;select.appendChild(option)});select.dataset.options=options;if([...select.options].some(o=>o.value===previous))select.value=previous}select.disabled=!select.options.length;if(practice&&s.mode!==2){$('practice-title').textContent='当前已切换玩法';$('practice-detail').textContent='点「返回手势识别」后继续挥杖练习。'}}
function appendLog(text){pending+=text;const lines=pending.split('\n');pending=lines.pop().slice(-2048);for(let line of lines){line=line.replace(/\r$/,'');if(!line)continue;logs.push(line);if(logs.length>180)logs.shift();const sample=line.match(/^(-?\d+\.?\d*),(-?\d+\.?\d*),(-?\d+)$/);if(sample){motion.push(sample.slice(1).map(Number));if(motion.length>200)motion.shift();scheduleCharts()}}const el=$('log-window');if(!$('advanced').hidden&&$('diagnostic-log').open){el.textContent=logs.join('\n');el.scrollTop=el.scrollHeight}}
let practiceCatalog='';
function practiceTargetPhrase(){const i=Number($('practice-target').value);return bindingState?.gestureProfile==='legacy-v1'&&i===5?'画一个竖着的 8':'画手势 '+(bindingState?.gestureLabels[i]||'')}
function updatePracticeTargets(){
 document.querySelectorAll('#practice-targets button').forEach(button=>{
  button.setAttribute('aria-pressed',String(button.value===$('practice-target').value));
  button.disabled=!!bindingState?.modelUpdating||Date.now()-lastStatus>10000;
 });
}
function syncPractice(s){if(practiceCatalog!==s.gestureProfile){practiceCatalog=s.gestureProfile;$('practice-target').replaceChildren(...s.gestureLabels.map((name,i)=>{const o=document.createElement('option');o.value=i;o.textContent=name;return o}));const lamp=s.bindings.indexOf(5);$('practice-target').value=String(lamp>=0?lamp:0);
 $('practice-targets').replaceChildren(...s.gestureLabels.map((name,i)=>{
  const button=document.createElement('button');
  button.type='button';button.textContent=name;button.value=i;button.dataset.device='';
  button.onclick=()=>{$('practice-target').value=button.value;updatePracticeTargets()};
  return button;
 }));
 }updatePracticeTargets();$('practice-target').disabled=!!s.modelUpdating;$('guide-action').textContent='练习时先选择当前模型中的手势。识别后，以你在“手势设置”保存的功能和灯效为准。'}
function showRecognition(result){if(!practice)return;clearTimeout(practiceTimer);const id=result.id;const label=bindingState?.gestureLabels[id]||names[id]||'未知手势';$('practice-title').textContent=(result.accepted?'识别到：':'未达到门槛，候选：')+label;const action=Number.isInteger(result.action)?result.action:bindingState?.bindings[id];$('practice-detail').textContent=!result.accepted?'先停稳，再在约1秒内画完，然后停稳。':action===-1?(result.effect>0?'已识别 · 灯效：'+(bindingState?.lightEffects.find(f=>f.id===result.effect)?.name||'自定义灯效'):'这个手势设为“不执行功能”，已识别，不执行动作。'):'对应功能：'+(names[action]||'未知')+(result.effect>0?' · 灯效：'+(bindingState?.lightEffects.find(f=>f.id===result.effect)?.name||'自定义灯效'):'')+'。请观察设备实际效果。'}
function bindingControls(){ $('save-bindings').hidden=$('reload-bindings').hidden=!bindingDirty&&!bindingSaving;const connected=ws?.readyState===WebSocket.OPEN&&Date.now()-lastStatus<=10000&&!bindingState?.modelUpdating;document.querySelectorAll('#gesture-mapping select').forEach(s=>s.disabled=!connected||bindingSaving||!bindingState?.bindingsWritable);document.querySelectorAll('.binding-preview').forEach((b,i)=>b.disabled=!connected||bindingSaving||!bindingEffectsDraft||bindingEffectsDraft[i]<=0);$('save-bindings').disabled=!connected||bindingSaving||!bindingDirty||!bindingState?.bindingsWritable;$('reload-bindings').disabled=!connected||bindingSaving||!bindingState;}
function bindingChanged(){updateBindingSummaries();bindingDirty=JSON.stringify(bindingDraft)!==JSON.stringify(bindingState.bindings)||JSON.stringify(bindingEffectsDraft)!==JSON.stringify(bindingState.gestureEffects);$('binding-feedback').textContent=bindingDirty?'有未保存的修改':'没有未保存的修改';bindingControls()}
function updateBindingSummaries(){
 document.querySelectorAll('.binding-row').forEach((row,i)=>{
  const action=names[bindingDraft[i]]||'不执行功能';
  const effect=bindingState.lightEffects.find(item=>item.id===bindingEffectsDraft[i])?.name||'默认灯效';
  row.querySelector('.binding-summary span').textContent=action+' · '+effect+'  ›';
 });
}
function renderBindings(s){
 if(!Array.isArray(s.bindings)||s.bindings.length<2||s.bindings.length>16||!Array.isArray(s.gestureLabels)||s.gestureLabels.length!==s.bindings.length||!Array.isArray(s.gestureEffects)||s.gestureEffects.length!==s.bindings.length||!Array.isArray(s.lightEffects)){$('binding-feedback').textContent='请先升级支持独立灯效的新固件';$('save-bindings').disabled=true;$('reload-bindings').disabled=true;return}
 s.bindings=s.bindings.map(action=>names[action]?action:-1);
 const changed=bindingState?.gestureProfile!==s.gestureProfile;bindingState=s;
 if(changed){bindingDirty=false;bindingDraft=[...s.bindings];bindingEffectsDraft=[...s.gestureEffects];$('gesture-mapping').replaceChildren();
 s.gestureLabels.forEach((name,i)=>{
  const row=document.createElement('div');row.className='binding-row';const glyph=document.createElement('strong');glyph.className='binding-gesture';glyph.textContent=name;
  const field=(caption,cls)=>{const f=document.createElement('div');f.className='binding-field '+cls;const label=document.createElement('label'),select=document.createElement('select');select.id='gesture-'+cls+'-'+i;select.setAttribute('aria-label',name+' 的'+caption);label.htmlFor=select.id;label.textContent=caption;f.append(label,select);return [f,select]};
  const [af,action]=field('功能','binding-action-field'),[ef,effect]=field('灯光特效','binding-effect-field');
  [{id:-1,name:'不执行功能'},...names.map((name,id)=>({id,name})).filter(action=>action.name)].forEach(a=>{const o=document.createElement('option');o.value=a.id;o.textContent=a.name;action.append(o)});
  let group;for(const fx of s.lightEffects){if(!group||group.label!==fx.group){group=document.createElement('optgroup');group.label=fx.group;effect.append(group)}const o=document.createElement('option');o.value=fx.id;o.textContent=fx.name;group.append(o)}
  action.onchange=()=>{bindingDraft[i]=Number(action.value);bindingChanged()};effect.onchange=()=>{bindingEffectsDraft[i]=Number(effect.value);bindingChanged()};
  const preview=document.createElement('button');preview.className='binding-preview';preview.dataset.device='';preview.title='选择具体灯效后可预览，不执行功能';preview.textContent='预览';preview.setAttribute('aria-label','预览 '+name+' 的灯效');
  const summary=document.createElement('button');
  summary.type='button';summary.className='binding-summary';summary.setAttribute('aria-expanded','false');
  const detail=document.createElement('span');
  summary.append(document.createTextNode(name),detail);
  summary.onclick=()=>{
   const opening=!row.hasAttribute('data-editing');
   const prior=$('gesture-mapping').querySelector('[data-editing]');
   if(prior){prior.removeAttribute('data-editing');prior.querySelector('.binding-summary').setAttribute('aria-expanded','false')}
   row.toggleAttribute('data-editing',opening);
   summary.setAttribute('aria-expanded',String(opening));
  };
  const feedback=document.createElement('p');
  feedback.className='binding-preview-feedback small';feedback.setAttribute('role','status');
  preview.onclick=()=>{if(send('web:effect '+bindingEffectsDraft[i])){previewFeedback=feedback;feedback.textContent='正在发送…'}};
  row.append(glyph,summary,af,ef,preview,feedback);$('gesture-mapping').append(row);
 })}
 if(!bindingDirty&&!bindingSaving){bindingDraft=[...s.bindings];bindingEffectsDraft=[...s.gestureEffects];bindingRevision=s.bindingRevision;document.querySelectorAll('.binding-row').forEach((row,i)=>{row.querySelector('.binding-action-field select').value=String(bindingDraft[i]);row.querySelector('.binding-effect-field select').value=String(bindingEffectsDraft[i])});if(!$('binding-feedback').textContent.startsWith('已保存'))$('binding-feedback').textContent='已读取配置，断电后保留'}
 updateBindingSummaries();
 $('binding-model').textContent='当前模型：'+s.gestureProfileTitle;$('binding-profile-note').hidden=s.gestureProfile!=='legacy-v1';$('binding-profile-note').textContent=s.gestureProfile!=='legacy-v1'?'功能和灯效可以分别选择':'当前是原版模型，下面是原来的8类手势。安装自己训练的模型后会显示字母或符号。';if(!s.bindingsWritable)$('binding-feedback').textContent='设备配置存储不可用，暂时不能保存';bindingControls()
}
$('save-bindings').onclick=()=>{if(!bindingDraft||!bindingDirty||bindingSaving)return;const command='web:bindings '+bindingRevision+' '+bindingDraft.join(',')+' '+bindingEffectsDraft.join(',');if(!send(command))return;bindingSaving=true;$('binding-feedback').textContent='正在保存到魔杖…';bindingControls();clearTimeout(bindingTimer);bindingTimer=setTimeout(()=>{bindingSaving=false;$('binding-feedback').textContent='尚未收到保存确认，请重新读取设备配置后检查。';bindingControls()},8000)};
$('reload-bindings').onclick=()=>{bindingDirty=false;bindingDraft=null;bindingEffectsDraft=null;$('binding-feedback').textContent='正在重新读取…';send('web:status')};
$('ir-add').onclick=()=>{const slot=irSlots.findIndex(item=>item.state===0);if(slot>=0)irCommand(0x41,slot);else toast('红外配件的存储位置已满')};
$('ir-scan').onclick=()=>irCommand(0x40,0);
$('ir-pair').onclick=()=>{const mac=$('ir-pair').dataset.mac;if(mac)send('web:ir-pair '+mac)};
$('ir-unpair').onclick=()=>send('web:ir-unpair');
$('ir-command-send').onclick=()=>{const slot=Number($('ir-command-slot').value);if(Number.isInteger(slot)&&slot>=0&&slot<32)irCommand(0x42,slot)};
let motion=[],chartFrame=0;
function scheduleCharts(){if($('advanced').hidden||$('capture-charts').hidden||chartFrame)return;chartFrame=requestAnimationFrame(()=>{chartFrame=0;drawChart('motion-chart',[0,1],10);drawChart('state-chart',[2],4)})}
function drawChart(id,indices,range){const canvas=$(id),width=canvas.clientWidth,height=canvas.clientHeight;if(!width)return;const dpr=window.devicePixelRatio||1;canvas.width=width*dpr;canvas.height=height*dpr;const ctx=canvas.getContext('2d');ctx.scale(dpr,dpr);const states=id==='state-chart';if(!states)for(const point of motion)range=Math.max(range,Math.abs(point[0]),Math.abs(point[1]));ctx.strokeStyle='#e0d4be';ctx.fillStyle='#807360';ctx.font='11px sans-serif';for(let n=0;n<=4;n++){const y=15+(height-35)*n/4;ctx.beginPath();ctx.moveTo(35,y);ctx.lineTo(width-10,y);ctx.stroke();ctx.fillText((states?range*(1-n/4):range*(1-n/2)).toFixed(0),4,y+4)}indices.forEach((index,series)=>{ctx.strokeStyle=['#b15549','#b48c3c'][series];ctx.lineWidth=1.5;ctx.beginPath();motion.forEach((point,n)=>{const x=35+(width-45)*n/199;const ratio=states?point[index]/range:(point[index]+range)/(range*2);const y=15+(height-35)*(1-ratio);if(n)ctx.lineTo(x,y);else ctx.moveTo(x,y)});ctx.stroke()})}
window.addEventListener('resize',scheduleCharts);
function endPractice(restore){practice=false;clearTimeout(practiceTimer);$('practice-start').hidden=false;$('practice-end').hidden=true;if(restore)send('back','练习已结束，正在恢复手势识别');$('practice-title').textContent=restore?'练习已结束':'等待练习';$('practice-detail').textContent='停稳 → 挥杖 → 停稳。识别成功会执行已绑定的动作。'}
document.querySelectorAll('[data-cmd]').forEach(button=>button.addEventListener('click',()=>send(button.dataset.cmd,button.dataset.message)));
$('practice-start').onclick=()=>{if(!send('back','正在切换到手势识别，停稳后开始练习'))return;practice=true;$('practice-start').hidden=true;$('practice-end').hidden=false;$('practice-title').textContent='试着'+practiceTargetPhrase();$('practice-detail').textContent='正在等待实际识别结果，动作前后都要停稳。';clearTimeout(practiceTimer);practiceTimer=setTimeout(()=>{if(practice){$('practice-title').textContent='还没收到识别结果';$('practice-detail').textContent='确认魔杖已连接，缩短动作时间，并在画完后停稳。'}},12000)};
$('practice-end').onclick=()=>endPractice(true);
$('apply-pattern').onclick=()=>{if(!$('pattern').value){toast('图案尚未读取，请等魔杖连接后重试');return}if(send('pov '+$('pattern').value+' '+($('reverse').checked?1:0),'已请求设置图案；进入光绘模式并挥动即可体验'))$('pattern-feedback').textContent='已发送设置：'+$('pattern').selectedOptions[0].textContent+'。点击「进入光绘」后挥动魔杖。'};
const actions={reboot:['重新启动魔杖？','网页会暂时断开。魔杖重新启动后，连接它的 Wi-Fi 即可继续。','正在重新启动，请稍后重连'],sleep:['让魔杖休息一会？','网页会断开。短按底部按键可唤醒，随后可能需要重新连接 Wi-Fi。','魔杖已请求休眠，短按底部按键后重连'],shutdown:['关闭魔杖？','灯光和无线连接都会关闭。下次单击底部按键即可开机。','已请求关机，单击底部按键可开机'],ap:['关闭魔杖 Wi-Fi？','当前网页会断开。需要再次使用网页时，重启魔杖即可恢复热点。','已请求关闭 Wi-Fi，重启可恢复'],ota:['进入固件升级？','当前指令页会断开。请保持供电，重连魔杖 Wi-Fi 后打开升级页，选择匹配的应用固件 .bin 文件上传。','已请求升级模式，请重连 Wi-Fi 并打开升级页']};
document.querySelectorAll('[data-action]').forEach(button=>button.onclick=()=>{confirmedAction=button.dataset.action;$('confirm-title').textContent=actions[confirmedAction][0];$('confirm-body').textContent=actions[confirmedAction][1];$('confirm-dialog').showModal()});
$('cancel-action').onclick=()=>$('confirm-dialog').close();$('confirm-action').onclick=()=>{const action=confirmedAction;if(send(action,actions[action][2])){if(action==='ota')$('ota-link').hidden=false;expectedDisconnect=actions[action][2];online(false)}$('confirm-dialog').close()};
$('diagnostic-log').addEventListener('toggle',()=>{if($('diagnostic-log').open)$('log-window').textContent=logs.join('\n')});
$('clear-log').onclick=()=>{logs=[];pending='';motion=[];scheduleCharts();$('log-window').textContent=''};
let modelUploading=false;$('model-file').onchange=()=>{$('upload-model-file').hidden=!$('model-file').files.length;$('upload-model-file').disabled=Date.now()-lastStatus>10000||modelUploading||!!bindingState?.modelUpdating||!$('model-file').files.length};
async function readModelInfo(){try{const r=await fetch('/api/model');if(!r.ok)return;const data=await r.json();$('model-info').textContent=(data.source==='stored'?'独立模型':'内置模型')+' · '+data.gestures.length+'个手势';return data}catch(_){}}
$('upload-model-file').onclick=async()=>{const file=$('model-file').files[0];if(!file||modelUploading)return;modelUploading=true;$('upload-model-file').disabled=true;$('model-upload-progress').hidden=false;$('model-upload-message').textContent='正在上传模型';try{const bytes=new Uint8Array(await file.arrayBuffer());if(bytes.length<1128)throw Error('请选择完整的 .wandmodel 模型包');const target=Array.from(bytes.slice(-32),v=>v.toString(16).padStart(2,'0')).join('');const current=await readModelInfo();if(current?.active_hash===target){$('model-upload-progress').value=100;$('model-upload-message').textContent='魔杖已在使用这个模型';return}const r=await fetch('/api/model',{method:'POST',headers:{'Content-Type':'application/octet-stream','X-Wand-Model':'1'},body:file});const answer=await r.json();if(r.status!==202)throw Error(answer.error||'上传未完成');const deadline=Date.now()+75000;while(Date.now()<deadline){await new Promise(r=>setTimeout(r,700));const data=await readModelInfo();if(!data)continue;if(data.active_hash===target){$('model-upload-progress').value=100;$('model-upload-message').textContent='新模型已生效';return}if(data.job?.ticket===answer.ticket){if(data.job.phase==='error')throw Error(data.job.message);$('model-upload-progress').value=data.job.percent;$('model-upload-message').textContent=data.job.message}}throw Error('请重连魔杖后检查模型状态')}catch(e){$('model-upload-message').textContent=e.message}finally{modelUploading=false;$('model-file').onchange()}};
let activePage='control';
const pages = [...document.querySelectorAll('[data-page]')];
const pageLinks = document.querySelectorAll('.page-nav a');
function showPage(event) {
 const active = pages.find(page => '#'+page.id===location.hash) || pages[0];
 const command = active.id==='control' && (location.hash==='#light-settings'||location.hash==='#remote');
 activePage=active.id;
 $('mode-bar').hidden=!lastStatus||(activePage!=='control'&&($('back-mode').hidden||(activePage==='advanced'&&!$('capture-stop').hidden)));
 for (const page of pages) page.hidden = page!==active;
 for (const link of pageLinks) {
  if (link.hash==='#'+active.id) link.setAttribute('aria-current','page');
  else if (link.hash==='#'+active.dataset.parent) link.setAttribute('aria-current','location');
  else link.removeAttribute('aria-current');
 }
 if (active.id!=='learn' && practice) endPractice(false);
 if (active.id==='advanced') {
  $('log-window').textContent=logs.join('\n');
  scheduleCharts();
 }
 if (active.id==='model-settings' && Date.now()-lastStatus<=10000) readModelInfo();
 document.title=active.querySelector('h1').textContent+' · 我的魔杖';
 if (event) {
  active.querySelector('h1').focus({preventScroll:true});
  if (command) $(location.hash.slice(1)).scrollIntoView({block:'nearest'});
  else window.scrollTo(0,0);
 }
}
window.addEventListener('hashchange',showPage);
showPage();
online(false);if(/^https?:$/.test(location.protocol)){$('connection-text').textContent='正在连接魔杖…';connect()}else{$('connection-text').textContent='离线界面预览';$('offline').textContent='离线预览。连接魔杖后可操作。'}
</script>
</body>
</html>
)raw";
