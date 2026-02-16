// ============================================================
//  BadUSB Control Panel — Frontend Logic
// ============================================================

const API = '';   // same origin

// --- DOM refs ---
const $ = (id) => document.getElementById(id);
const payloadList  = $('payloadList');
const payloadName  = $('payloadName');
const editor       = $('editor');
const liveEditor   = $('liveEditor');
const statusDot    = $('statusDot');
const statusText   = $('statusText');
const autorunSel   = $('autorunSelect');
const storageInfo  = $('storageInfo');

let currentPayload = '';
let autorunPayload = '';

// ================================================================
//  API Helpers
// ================================================================

async function api(method, path, body = null) {
    const opts = { method, headers: { 'Content-Type': 'application/json' } };
    if (body) opts.body = JSON.stringify(body);
    const res = await fetch(API + path, opts);
    return res.json();
}

// ================================================================
//  Toast Notifications
// ================================================================

function toast(msg, type = 'info') {
    let container = document.querySelector('.toast-container');
    if (!container) {
        container = document.createElement('div');
        container.className = 'toast-container';
        document.body.appendChild(container);
    }
    const el = document.createElement('div');
    el.className = `toast ${type}`;
    el.textContent = msg;
    container.appendChild(el);
    setTimeout(() => el.remove(), 3000);
}

// ================================================================
//  Payloads
// ================================================================

async function loadPayloads() {
    try {
        const data = await api('GET', '/api/payloads');
        const payloads = data.payloads || [];

        payloadList.innerHTML = '';
        autorunSel.innerHTML = '<option value="">— None —</option>';

        payloads.forEach(name => {
            // Sidebar item
            const div = document.createElement('div');
            div.className = 'payload-item' + (name === currentPayload ? ' active' : '');
            div.innerHTML = name + (name === autorunPayload ?
                ' <span class="autorun-badge">AUTO</span>' : '');
            div.onclick = () => selectPayload(name);
            payloadList.appendChild(div);

            // Autorun dropdown
            const opt = document.createElement('option');
            opt.value = name;
            opt.textContent = name;
            if (name === autorunPayload) opt.selected = true;
            autorunSel.appendChild(opt);
        });
    } catch (e) {
        toast('Failed to load payloads', 'error');
    }
}

async function selectPayload(name) {
    try {
        const data = await api('GET', `/api/payloads/${encodeURIComponent(name)}`);
        currentPayload = name;
        payloadName.value = name;
        editor.value = data.content || '';
        loadPayloads();   // refresh active state
    } catch (e) {
        toast('Failed to load payload', 'error');
    }
}

async function savePayload() {
    const name = payloadName.value.trim();
    if (!name) { toast('Enter a payload name', 'error'); return; }
    try {
        await api('POST', '/api/payloads', { name, content: editor.value });
        currentPayload = name;
        toast('Payload saved!', 'success');
        loadPayloads();
    } catch (e) {
        toast('Save failed', 'error');
    }
}

async function deletePayload() {
    if (!currentPayload) return;
    if (!confirm(`Delete "${currentPayload}"?`)) return;
    try {
        await api('DELETE', `/api/payloads/${encodeURIComponent(currentPayload)}`);
        toast('Deleted', 'success');
        currentPayload = '';
        payloadName.value = '';
        editor.value = '';
        loadPayloads();
    } catch (e) {
        toast('Delete failed', 'error');
    }
}

// ================================================================
//  Execution
// ================================================================

async function runPayload() {
    if (!currentPayload) { toast('Select a payload first', 'error'); return; }
    try {
        await api('POST', `/api/execute/${encodeURIComponent(currentPayload)}`);
        toast('Executing...', 'info');
        pollStatus();
    } catch (e) {
        toast('Execution failed', 'error');
    }
}

async function runLive() {
    const script = liveEditor.value.trim();
    if (!script) { toast('Enter script commands', 'error'); return; }
    try {
        await api('POST', '/api/execute/live', { script });
        toast('Live executing...', 'info');
        pollStatus();
    } catch (e) {
        toast('Execution failed', 'error');
    }
}

async function stopExecution() {
    try {
        await api('POST', '/api/stop');
        toast('Stopped', 'success');
    } catch (e) {
        toast('Stop failed', 'error');
    }
}

// ================================================================
//  Status Polling
// ================================================================

let pollTimer = null;

function pollStatus() {
    if (pollTimer) clearInterval(pollTimer);
    pollTimer = setInterval(async () => {
        try {
            const data = await api('GET', '/api/status');
            updateStatusUI(data);
            if (!data.running && pollTimer) {
                clearInterval(pollTimer);
                pollTimer = null;
            }
        } catch (e) { /* ignore */ }
    }, 500);
}

function updateStatusUI(data) {
    const running = data.running;
    statusDot.className = 'status-indicator' + (running ? ' running' : '');
    statusText.textContent = running ? 'Executing...' : 'Idle';
    $('btnRun').disabled  = running;
    $('btnStop').disabled = !running;

    // Storage info
    if (data.storage) {
        const pct = Math.round((data.storage.used / data.storage.total) * 100);
        const free = (data.storage.free / 1024).toFixed(0);
        storageInfo.innerHTML = `
            Storage: ${pct}% used (${free} KB free)
            <div class="storage-bar"><div class="storage-bar-fill" style="width:${pct}%"></div></div>
        `;
    }

    // SSID / IP
    if (data.ssid) $('infoSSID').textContent = data.ssid;
    if (data.ip)   $('infoIP').textContent   = data.ip;
    if (data.storage) {
        $('infoStorage').textContent = `${(data.storage.used/1024).toFixed(0)}/${(data.storage.total/1024).toFixed(0)} KB`;
    }

    if (data.autorun !== undefined) {
        autorunPayload = data.autorun || '';
    }
}

// ================================================================
//  Settings
// ================================================================

async function setAutorun() {
    const name = autorunSel.value;
    try {
        await api('POST', '/api/settings', { autorun: name });
        autorunPayload = name;
        toast(name ? `Auto-run: ${name}` : 'Auto-run disabled', 'success');
        loadPayloads();
    } catch (e) {
        toast('Failed to set auto-run', 'error');
    }
}

// ================================================================
//  Templates
// ================================================================

const TEMPLATES = {
    notepad: `REM Open Notepad and type a message
DELAY 1000
GUI r
DELAY 500
STRING notepad
ENTER
DELAY 1000
STRING Hello from BadUSB!
`,
    browser: `REM Open browser to a URL
DELAY 1000
GUI r
DELAY 500
STRING https://example.com
ENTER
`,
    terminal: `REM Open Command Prompt
DELAY 1000
GUI r
DELAY 500
STRING cmd
ENTER
DELAY 1000
STRING echo BadUSB Connected!
ENTER
`
};

function loadTemplate(name) {
    editor.value = TEMPLATES[name] || '';
    payloadName.value = name + '.ducky';
}

// ================================================================
//  Init
// ================================================================

document.addEventListener('DOMContentLoaded', () => {
    // Button handlers
    $('btnSave').onclick   = savePayload;
    $('btnRun').onclick    = runPayload;
    $('btnStop').onclick   = stopExecution;
    $('btnDelete').onclick = deletePayload;
    $('btnLive').onclick   = runLive;
    $('btnAutorun').onclick = setAutorun;
    $('btnNew').onclick    = () => {
        currentPayload = '';
        payloadName.value = '';
        editor.value = '';
        loadPayloads();
    };

    // Templates
    $('btnTemplate1').onclick = () => loadTemplate('notepad');
    $('btnTemplate2').onclick = () => loadTemplate('browser');
    $('btnTemplate3').onclick = () => loadTemplate('terminal');

    // Tab key support in editor
    editor.addEventListener('keydown', (e) => {
        if (e.key === 'Tab') {
            e.preventDefault();
            const start = editor.selectionStart;
            const end   = editor.selectionEnd;
            editor.value = editor.value.substring(0, start) + '    ' + editor.value.substring(end);
            editor.selectionStart = editor.selectionEnd = start + 4;
        }
    });

    // Keyboard shortcut: Ctrl+S to save
    document.addEventListener('keydown', (e) => {
        if ((e.ctrlKey || e.metaKey) && e.key === 's') {
            e.preventDefault();
            savePayload();
        }
    });

    // Initial load
    loadPayloads();

    // Poll status every 2 seconds
    setInterval(async () => {
        try {
            const data = await api('GET', '/api/status');
            updateStatusUI(data);
        } catch (e) { /* ignore */ }
    }, 2000);
});
