"""Browser checks using simulated firmware messages; does not replace hardware testing.
Run with a Python environment containing playwright and an installed Chromium:
  python tools/smoke_test_web.py --browser /path/to/chromium --output /tmp/wand-ui
"""
import argparse
import json
from pathlib import Path
from playwright.sync_api import sync_playwright

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--browser', required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
source = Path(__file__).resolve().parents[1] / 'src/Service/WebPage.h'
html = source.read_text().split('R"raw(\n', 1)[1].rsplit(')raw";', 1)[0]
status = dict(type='status', version='5.1', ssid="Muggles' Wand", password='12345678',
              bleName="Muggles' Wand", mode=2, bleEnabled=True, bleConnected=False,
              charging=False, receiver=True, patterns=['MugglesWand', 'Star', 'Rainbow'],
              gestureProfile='legacy-v1', gestureProfileTitle='原版手势模型',
              gestureLabels=['关灯', '开灯', '向上', '向下', '鼠标', '照明魔法', '音量', '光绘'],
              bindings=[-1,-1,-1,-1,4,5,6,7], gestureEffects=[-1]*8, lightEffects=[],
              bindingRevision=1, bindingsWritable=True,
              irSlots=[dict(state=1 if i == 0 else 0, online=False,
                            name='客厅开关' if i == 0 else '') for i in range(32)],
              irLink=dict(paired=None, candidate='80:53:E0:05:EB:C8', online=False, writable=True))
commands, sockets, errors = [], [], []

def device(socket):
    sockets.append(socket)
    def receive(command):
        if command.startswith('web:ha-config\n'):
            assert command.split('\n')[1:] == [
                '1', 'Test Wi-Fi', 'fixture-wifi', '192.168.1.20', '1883', 'wand', 'fixture-mqtt',
            ]
            commands.append('web:ha-config')
            socket.send(json.dumps(dict(type='haConfig', ok=True)))
            return
        commands.append(command)
        if command == 'web:status':
            socket.send(json.dumps(status))
        elif command.startswith('web:ir-pair '):
            assert command == 'web:ir-pair 80:53:E0:05:EB:C8'
            status['irLink'].update(paired='80:53:E0:05:EB:C8', candidate=None, online=True)
            for slot in status['irSlots']: slot['online'] = True
            socket.send(json.dumps(dict(type='irPair', ok=True)))
            socket.send(json.dumps(status))
        elif command == 'web:ir-unpair':
            status['irLink'].update(paired=None, candidate=None, online=False)
            for slot in status['irSlots']: slot['online'] = False
            socket.send(json.dumps(dict(type='irPair', ok=True)))
            socket.send(json.dumps(status))
        elif command in ('back', 'inference'):
            status['mode'] = 2
            socket.send(json.dumps(status))
        elif command == 'web:ha-test':
            socket.send(json.dumps(dict(type='haTest', ok=True)))
        elif command == 'imu':
            status['mode'] = 1
            socket.send(json.dumps(status))
        elif command == 'stop':
            status['mode'] = -1
            socket.send(json.dumps(status))
        elif command.startswith('gesture '):
            status['mode'] = {4: 3, 6: 4, 7: 5}.get(int(command.split()[1]), 2)
            socket.send(json.dumps(status))
        elif command == 'ble':
            status['bleEnabled'] = not status['bleEnabled']
            socket.send(json.dumps(status))
        elif command in ('reboot', 'sleep', 'shutdown', 'ap', 'ota'):
            socket.close()
    socket.on_message(receive)

with sync_playwright() as p:
    browser = p.chromium.launch(executable_path=args.browser, args=['--no-sandbox'])
    page = browser.new_page(viewport={'width': 1440, 'height': 1050}, device_scale_factor=1)
    page.on('pageerror', lambda error: errors.append(str(error)))
    page.route('http://wand.test/**', lambda route: route.fulfill(body=html, content_type='text/html'))
    page.route_web_socket('ws://wand.test/ws', device)
    page.goto('http://wand.test/')
    page.get_by_text('已连接魔杖', exact=True).wait_for()
    assert page.locator('#mode').inner_text() == '挥杖识别已就绪'
    assert page.locator('#bluetooth').text_content() == '已开启，等待配对'
    assert page.locator('#pattern').input_value() == '0'
    assert page.locator('[data-page]:visible').get_attribute('id') == 'control'
    assert page.locator('.page-nav a').all_text_contents() == ['发送指令', '配置', '设备']
    for action in (5, 7, 4, 6):
        assert page.locator(f'#control [data-cmd="gesture {action}"]').is_visible()
    assert page.get_by_role('heading', name='外部开关').count() == 0
    assert page.get_by_role('heading', name='红外配件').is_visible()
    page.get_by_role('link', name='配置', exact=True).click()
    page.locator('#settings').wait_for()
    assert page.locator('#control').is_hidden()
    assert page.get_by_role('link', name='手势与灯效').is_visible()
    page.get_by_role('link', name='红外配件').click()
    page.locator('#ir-pair').click()
    page.get_by_text('已连接到 80:53:E0:05:EB:C8').wait_for()
    page.locator('#ir-settings').screenshot(path=str(args.output / 'ir-settings.png'))
    page.get_by_role('link', name='发送指令', exact=True).click()
    page.screenshot(path=str(args.output / 'product-control.png'), full_page=True)
    page.locator('#ir-command-send').click()
    assert commands[-1] == 'web:ir 66 0'
    page.get_by_role('link', name='配置', exact=True).click()
    page.get_by_role('link', name='红外配件').click()
    page.locator('#ir-signals .ir-slot').first.wait_for()
    assert page.locator('#ir-signals .ir-slot').count() == 1
    page.locator('#ir-signals .ir-slot').first.get_by_role('button', name='重放').click()
    assert commands[-1] == 'web:ir 66 0'
    page.locator('#ir-add').click()
    assert commands[-1] == 'web:ir 65 1'
    status['irSlots'][1].update(state=3, name='空调开关')
    sockets[-1].send(json.dumps(status))
    page.get_by_text('学习成功，已保存到 C3').wait_for()
    for slot in status['irSlots'][2:9]: slot.update(state=1, online=True, name='已学信号')
    sockets[-1].send(json.dumps(status))
    assert page.locator('#ir-signals .ir-slot').count() == 9
    page.locator('#ir-add').click()
    assert commands[-1] == 'web:ir 65 9'
    page.get_by_role('link', name='返回配置').click()
    page.get_by_role('link', name='红外配件').click()
    page.locator('#ir-unpair').click()
    page.locator('#ir-link-state').filter(has_text='正在搜索 C3').wait_for()
    assert page.locator('#ir-signals .ir-slot').first.get_by_role('button', name='重放').is_disabled()
    page.get_by_role('link', name='返回配置').click()
    page.get_by_role('link', name='手势与灯效').click()
    assert page.locator('.binding-action-field select').first.locator('option[value="13"]').text_content() == '空调开关'
    page.get_by_role('link', name='返回配置').click()
    page.get_by_role('link', name='更新手势模型').wait_for(state='visible')
    page.get_by_role('link', name='Home Assistant', exact=False).click()
    page.locator('#ha-settings').wait_for()
    assert page.locator('#ha-fields').is_hidden()
    assert page.locator('#ha-reboot').is_hidden()
    page.locator('#ha-enabled').check()
    page.locator('#ha-ssid').fill('Test Wi-Fi')
    page.locator('#ha-wifi-key').fill('fixture-wifi')
    page.locator('#ha-broker').fill('192.168.1.20')
    page.locator('#ha-user').fill('wand')
    page.locator('#ha-key').fill('fixture-mqtt')
    page.locator('#ha-save').click()
    page.get_by_text('已保存。重启魔杖后生效。').wait_for()
    assert page.locator('#ha-reboot').is_visible()
    assert page.locator('#ha-save').is_hidden()
    assert page.locator('#ha-wifi-key').input_value() == page.locator('#ha-key').input_value() == ''
    page.get_by_role('link', name='发送指令', exact=True).click()
    page.locator('#control').wait_for()
    assert page.locator('#ha-test').is_disabled()
    status.update(haState=4, receiverConflict=True)
    sockets[-1].send(json.dumps(status))
    page.locator('#ha-test').click()
    page.get_by_text('测试事件已发送，请到 Home Assistant 查看。').wait_for()
    page.locator('#receiver-status').filter(has_text='信道冲突').wait_for(state='attached')
    assert page.locator('#ir-command-send').is_hidden()
    page.get_by_text('选择图案', exact=True).click()
    page.get_by_role('button', name='应用图案', exact=True).click()
    assert commands[-1] == 'pov 0 0'
    page.get_by_role('button', name='进入光绘', exact=True).click()
    assert commands[-1] == 'gesture 7'
    page.get_by_role('button', name='切换照明', exact=True).click()
    assert commands[-1] == 'gesture 5'
    page.get_by_role('button', name='进入鼠标模式', exact=True).click()
    page.wait_for_function("document.getElementById('mode').textContent==='隔空鼠标'")
    assert page.locator('[data-mode="3"]').get_attribute('data-active') == 'true'
    page.get_by_role('button', name='返回手势识别', exact=True).click()
    page.wait_for_function("document.getElementById('mode').textContent==='挥杖识别已就绪'")
    page.locator('#pattern').select_option('1')
    page.locator('#reverse').check()
    page.get_by_role('button', name='应用图案', exact=True).click()
    assert commands[-1] == 'pov 1 1'
    page.get_by_role('link', name='挥杖练习', exact=True).click()
    page.locator('#learn').wait_for()
    page.locator('#practice-start').click()
    # Near-threshold rejection must remain a rejection, even if rounded logs say P:0.60.
    sockets[-1].send(json.dumps(dict(type='recognition', id=5, action=5, confidence=0.5996, accepted=False)))
    page.get_by_text('未达到门槛，候选：照明魔法', exact=True).wait_for()
    sockets[-1].send(json.dumps(dict(type='recognition', id=5, action=5, confidence=0.89, accepted=True)))
    page.get_by_text('识别到：照明魔法', exact=True).wait_for()
    page.locator('#practice-end').click()
    page.get_by_role('link', name='设备', exact=True).click()
    page.locator('#device').wait_for()
    assert page.locator('#device #version').text_content() == status['version']
    assert page.locator('#device #bluetooth').is_visible()
    page.get_by_role('link', name='采集与诊断', exact=True).click()
    page.locator('#advanced').wait_for()
    assert page.locator('.page-nav a[href="#device"]').get_attribute('aria-current') == 'location'
    page.get_by_role('button', name='采集动作数据', exact=True).click()
    page.wait_for_function("document.getElementById('mode').textContent==='动作数据采集中'")
    page.locator('#capture-charts').wait_for()
    page.locator('#diagnostic-log > summary').click()
    # Multiple lines and a partial line arrive independently; no HTML execution.
    sockets[-1].send('1.2,2.3,1\n2.2,3.2,4\n<script>alert(1)</script>\npar')
    sockets[-1].send('tial\n')
    page.wait_for_function("document.getElementById('log-window').textContent.includes('partial')")
    assert '<script>alert(1)</script>' in page.locator('#log-window').inner_text()
    page.get_by_role('button', name='结束采集 / 恢复施法', exact=True).click()
    page.wait_for_function("document.getElementById('mode').textContent==='挥杖识别已就绪'")
    page.get_by_role('link', name='返回设备', exact=True).click()
    page.get_by_role('link', name='电源与 Wi-Fi', exact=True).click()
    before = commands.count('shutdown')
    page.get_by_role('button', name='关闭魔杖', exact=True).click()
    page.get_by_role('button', name='取消', exact=True).click()
    assert commands.count('shutdown') == before
    page.get_by_role('button', name='关闭魔杖', exact=True).click()
    page.locator('#confirm-action').click()
    page.wait_for_function("document.querySelector('[data-cmd=\"gesture 5\"]').disabled")
    assert commands.count('shutdown') == before + 1
    page.get_by_text('已连接魔杖', exact=True).wait_for(timeout=8000)
    # Device names must render as text, and pattern lists must come from the firmware.
    status['ssid'] = '<b>自定义魔杖</b>'
    status['patterns'] = ['Star', '<img src=x onerror=alert(1)>']
    sockets[-1].send(json.dumps(status))
    page.wait_for_function("document.getElementById('wifi-name').textContent==='<b>自定义魔杖</b>'")
    assert page.locator('#wifi-name b').count() == 0
    assert page.locator('#pattern option').count() == 2
    status['ssid'] = "Muggles' Wand"
    status['patterns'] = ['MugglesWand', 'Star', 'Rainbow']
    sockets[-1].send(json.dumps(status))
    page.get_by_role('link', name='发送指令', exact=True).click()
    page.locator('#control').wait_for()
    page.evaluate("document.getElementById('toast').hidden=true")
    page.screenshot(path=str(args.output / 'desktop.png'), full_page=True)
    for width in (768, 390, 320):
        page.set_viewport_size({'width': width, 'height': 844})
        for section in page.locator('[data-page]').evaluate_all('pages => pages.map(page => page.id)'):
            page.evaluate('(section) => location.hash=section', section)
            page.locator('#'+section).wait_for()
            assert page.locator('[data-page]:visible').count() == 1
            assert page.evaluate('document.documentElement.scrollWidth <= innerWidth'), (width, section)
        page.get_by_role('link', name='发送指令', exact=True).click()
        page.locator('#control').wait_for()
        assert page.get_by_role('button', name='切换照明').bounding_box()['y'] < 600
        if width == 390:
            page.screenshot(path=str(args.output / 'mobile.png'), full_page=True)
    assert len(sockets) == 2  # Navigation keeps one socket, including after reconnect.
    # Confirm disconnected UI has no active device controls.
    page.unroute_web_socket('ws://wand.test/ws', device) if hasattr(page, 'unroute_web_socket') else None
    sockets[-1].close()
    page.wait_for_function("document.querySelector('[data-cmd=\"gesture 5\"]').disabled")
    assert page.locator('#offline').is_visible()
    assert not errors, errors
    preview = args.output / 'preview.html'
    preview.write_text(html)
    page.goto(preview.resolve().as_uri())
    page.get_by_text('离线界面预览', exact=True).wait_for()
    assert page.locator('[data-device]:not(:disabled)').count() == 0
    assert not errors, errors
    ota_html = (source.parent / 'OTA.cpp').read_text(encoding='utf-8').split('R"page(', 1)[1].split(')page"', 1)[0]
    page.route('http://wand.test/update', lambda route: route.fulfill(body=ota_html, content_type='text/html'))
    page.route('**/ota/start?mode=firmware', lambda route: route.fulfill(body='OK'))
    attempts = []
    def update(route):
        attempts.append(route.request.method)
        if len(attempts) == 1:
            route.fulfill(status=400, body='固件校验失败', content_type='text/plain; charset=utf-8')
        else:
            route.fulfill(body='OK')
    page.route('**/ota/upload', update)
    page.goto('http://wand.test/update')
    assert page.locator('#send').is_hidden()
    page.locator('#file').set_input_files(dict(name='wrong.wandmodel', mimeType='application/octet-stream', buffer=b'model'))
    assert page.locator('#send').is_hidden()
    page.locator('#file').set_input_files(dict(name='app.bin', mimeType='application/octet-stream', buffer=b'firmware'))
    page.get_by_role('button', name='上传并更新').click()
    page.get_by_text('固件校验失败', exact=True).wait_for()
    page.get_by_role('button', name='重试上传').click()
    page.get_by_text('更新完成。请重连魔杖 Wi-Fi，再打开后台。').wait_for()
    assert attempts == ['POST', 'POST']
    assert page.locator('#progress').get_attribute('value') == '100'
    assert not errors, errors
    browser.close()
print('PASS: commands, status, mode return, pattern selection, real-result feedback, logs/charts, confirmation, reconnection, offline controls, custom names, mobile layout; no browser errors.')
