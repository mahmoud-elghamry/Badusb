const API = '';
const TOKEN_KEY = 'hidlab.adminToken';

const $ = (id) => document.getElementById(id);

const refs = {
    payloadList: $('payloadList'),
    payloadName: $('payloadName'),
    editor: $('editor'),
    liveEditor: $('liveEditor'),
    statusDot: $('statusDot'),
    statusText: $('statusText'),
    autorunSel: $('autorunSelect'),
    storageInfo: $('storageInfo'),
    usbVid: $('usbVid'),
    usbPid: $('usbPid'),
    authPanel: $('authPanel'),
    authTitle: $('authTitle'),
    authHint: $('authHint'),
    adminToken: $('adminToken'),
    mainContent: $('mainContent'),
};

let currentPayload = '';
let autorunPayload = '';
let adminToken = localStorage.getItem(TOKEN_KEY) || '';
let authenticated = false;
let setupRequired = false;
let hidReady = false;
let running = false;
let statusTimer = null;

class ApiError extends Error {
    constructor(status, message) {
        super(message);
        this.status = status;
    }
}

async function api(method, path, body = null, options = {}) {
    const headers = { 'Content-Type': 'application/json' };
    if (adminToken && options.auth !== false) {
        headers['X-Admin-Token'] = adminToken;
    }

    const res = await fetch(API + path, {
        method,
        headers,
        body: body === null ? null : JSON.stringify(body),
    });

    let data = {};
    const text = await res.text();
    if (text) {
        try {
            data = JSON.parse(text);
        } catch (e) {
            throw new ApiError(res.status, 'Invalid JSON response');
        }
    }

    if (!res.ok) {
        if (res.status === 401) {
            authenticated = false;
            showAuthPanel();
        }
        throw new ApiError(res.status, data.error || `HTTP ${res.status}`);
    }
    return data;
}

function toast(message, type = 'info') {
    let container = document.querySelector('.toast-container');
    if (!container) {
        container = document.createElement('div');
        container.className = 'toast-container';
        document.body.appendChild(container);
    }

    const el = document.createElement('div');
    el.className = `toast ${type}`;
    el.textContent = message;
    container.appendChild(el);
    setTimeout(() => el.remove(), 3200);
}

function isValidPayloadName(name) {
    return /^[A-Za-z0-9][A-Za-z0-9._-]{0,47}$/.test(name) && !name.includes('..');
}

function setBusyState(isRunning) {
    running = Boolean(isRunning);
    $('btnRun').disabled = running || !authenticated || !hidReady || !currentPayload;
    $('btnLive').disabled = running || !authenticated || !hidReady;
    $('btnStop').disabled = !running || !authenticated;
    $('btnSave').disabled = !authenticated;
    $('btnDelete').disabled = !authenticated || !currentPayload;
    $('btnAutorun').disabled = !authenticated;
    $('btnSaveUsbId').disabled = !authenticated;
}

function showAuthPanel() {
    refs.authPanel.classList.add('visible');
    refs.mainContent.classList.add('locked');
    refs.authTitle.textContent = setupRequired ? 'Admin Setup' : 'Admin Token';
    refs.authHint.textContent = setupRequired
        ? 'Create the local admin token.'
        : 'Enter the local admin token.';
    $('btnAuth').textContent = setupRequired ? 'Create' : 'Unlock';
    $('btnLogout').disabled = !adminToken;
}

function hideAuthPanel() {
    refs.authPanel.classList.remove('visible');
    refs.mainContent.classList.remove('locked');
}

function updateStatusUI(data) {
    setupRequired = Boolean(data.setupRequired);
    authenticated = Boolean(data.authenticated);
    hidReady = Boolean(data.hidReady);
    running = Boolean(data.running);

    refs.statusDot.className = 'status-indicator';
    if (data.duckyStatus === 'error') {
        refs.statusDot.classList.add('error');
    } else if (running) {
        refs.statusDot.classList.add('running');
    } else if (authenticated) {
        refs.statusDot.classList.add('ready');
    }

    if (setupRequired) {
        refs.statusText.textContent = 'Setup required';
    } else if (!authenticated) {
        refs.statusText.textContent = 'Locked';
    } else if (running) {
        refs.statusText.textContent = 'Executing';
    } else if (!hidReady) {
        refs.statusText.textContent = 'Config mode';
    } else {
        refs.statusText.textContent = 'Ready';
    }

    $('infoSSID').textContent = data.ssid || '-';
    $('infoIP').textContent = data.ip || '-';
    $('infoHID').textContent = hidReady ? 'Active' : 'Inactive';
    $('infoError').textContent = data.lastError || '-';

    if (data.storage) {
        const total = Number(data.storage.total || 0);
        const used = Number(data.storage.used || 0);
        const free = Number(data.storage.free || 0);
        const pct = total > 0 ? Math.min(100, Math.round((used / total) * 100)) : 0;
        refs.storageInfo.textContent = `Storage: ${pct}% used (${Math.round(free / 1024)} KB free)`;
        const bar = document.createElement('div');
        bar.className = 'storage-bar';
        const fill = document.createElement('div');
        fill.className = 'storage-bar-fill';
        fill.style.width = `${pct}%`;
        bar.appendChild(fill);
        refs.storageInfo.appendChild(bar);
        $('infoStorage').textContent = `${Math.round(used / 1024)}/${Math.round(total / 1024)} KB`;
    } else {
        refs.storageInfo.textContent = '';
        $('infoStorage').textContent = '-';
    }

    if (data.autorun !== undefined) {
        autorunPayload = data.autorun || '';
    }

    if (setupRequired || !authenticated) {
        showAuthPanel();
    } else {
        hideAuthPanel();
    }

    setBusyState(running);
}

async function refreshStatus() {
    const data = await api('GET', '/api/status', null, { auth: true });
    updateStatusUI(data);
    return data;
}

async function loadPayloads() {
    if (!authenticated) return;

    const data = await api('GET', '/api/payloads');
    const payloads = data.payloads || [];
    refs.payloadList.replaceChildren();
    refs.autorunSel.replaceChildren(new Option('None', ''));

    payloads.forEach((name) => {
        const item = document.createElement('button');
        item.type = 'button';
        item.className = 'payload-item';
        if (name === currentPayload) {
            item.classList.add('active');
        }

        const label = document.createElement('span');
        label.textContent = name;
        item.appendChild(label);

        if (name === autorunPayload) {
            const badge = document.createElement('span');
            badge.className = 'autorun-badge';
            badge.textContent = 'AUTO';
            item.appendChild(badge);
        }

        item.onclick = () => selectPayload(name);
        refs.payloadList.appendChild(item);

        const opt = new Option(name, name);
        opt.selected = name === autorunPayload;
        refs.autorunSel.add(opt);
    });
}

async function selectPayload(name) {
    try {
        const data = await api('GET', `/api/payloads/${encodeURIComponent(name)}`);
        currentPayload = name;
        refs.payloadName.value = name;
        refs.editor.value = data.content || '';
        await loadPayloads();
        setBusyState(running);
    } catch (e) {
        toast(e.message || 'Failed to load payload', 'error');
    }
}

async function savePayload() {
    const name = refs.payloadName.value.trim();
    if (!isValidPayloadName(name)) {
        toast('Invalid payload name', 'error');
        return;
    }

    try {
        await api('POST', '/api/payloads', { name, content: refs.editor.value });
        currentPayload = name;
        toast('Payload saved', 'success');
        await loadPayloads();
        setBusyState(running);
    } catch (e) {
        toast(e.message || 'Save failed', 'error');
    }
}

async function deletePayload() {
    if (!currentPayload || !confirm(`Delete "${currentPayload}"?`)) return;

    try {
        await api('DELETE', `/api/payloads/${encodeURIComponent(currentPayload)}`);
        currentPayload = '';
        refs.payloadName.value = '';
        refs.editor.value = '';
        toast('Payload deleted', 'success');
        await loadPayloads();
        setBusyState(running);
    } catch (e) {
        toast(e.message || 'Delete failed', 'error');
    }
}

async function runPayload() {
    if (!currentPayload) {
        toast('Select a payload first', 'error');
        return;
    }

    try {
        await api('POST', `/api/execute/${encodeURIComponent(currentPayload)}`);
        toast('Execution started', 'info');
        await refreshStatus();
    } catch (e) {
        toast(e.message || 'Execution failed', 'error');
    }
}

async function runLive() {
    const script = refs.liveEditor.value.trim();
    if (!script) {
        toast('Enter script commands', 'error');
        return;
    }

    try {
        await api('POST', '/api/execute/live', { script });
        toast('Live execution started', 'info');
        await refreshStatus();
    } catch (e) {
        toast(e.message || 'Execution failed', 'error');
    }
}

async function stopExecution() {
    try {
        await api('POST', '/api/stop');
        toast('Stop requested', 'success');
        await refreshStatus();
    } catch (e) {
        toast(e.message || 'Stop failed', 'error');
    }
}

async function loadSettings() {
    if (!authenticated) return;

    const data = await api('GET', '/api/settings');
    refs.usbVid.value = data.vid || '';
    refs.usbPid.value = data.pid || '';
    autorunPayload = data.autorun || '';
}

async function setAutorun() {
    try {
        const name = refs.autorunSel.value;
        await api('POST', '/api/settings', { autorun: name });
        autorunPayload = name;
        toast(name ? `Auto-run: ${name}` : 'Auto-run disabled', 'success');
        await loadPayloads();
    } catch (e) {
        toast(e.message || 'Failed to set auto-run', 'error');
    }
}

async function saveUsbId() {
    const vid = refs.usbVid.value.trim();
    const pid = refs.usbPid.value.trim();
    if (!/^[0-9A-Fa-f]{4}$/.test(vid) || !/^[0-9A-Fa-f]{4}$/.test(pid)) {
        toast('VID and PID must be 4 hex digits', 'error');
        return;
    }

    try {
        await api('POST', '/api/settings', { vid, pid });
        toast('USB ID saved', 'success');
    } catch (e) {
        toast(e.message || 'Failed to save USB ID', 'error');
    }
}

async function unlockOrSetup() {
    const token = refs.adminToken.value.trim();
    if (token.length < 10) {
        toast('Token must be at least 10 characters', 'error');
        return;
    }

    try {
        if (setupRequired) {
            await api('POST', '/api/setup', { token }, { auth: false });
        }
        adminToken = token;
        localStorage.setItem(TOKEN_KEY, adminToken);
        refs.adminToken.value = '';
        const status = await refreshStatus();
        if (!status.authenticated) {
            throw new ApiError(401, 'Invalid admin token');
        }
        await loadSettings();
        await loadPayloads();
        toast('Unlocked', 'success');
    } catch (e) {
        localStorage.removeItem(TOKEN_KEY);
        adminToken = '';
        authenticated = false;
        showAuthPanel();
        toast(e.message || 'Unlock failed', 'error');
    }
}

function logout() {
    localStorage.removeItem(TOKEN_KEY);
    adminToken = '';
    authenticated = false;
    currentPayload = '';
    refs.payloadName.value = '';
    refs.editor.value = '';
    refs.payloadList.replaceChildren();
    showAuthPanel();
    setBusyState(false);
}

const TEMPLATES = {
    notepad: `REM Open Notepad and type a message
DELAY 1000
GUI r
DELAY 500
STRING notepad
ENTER
DELAY 1000
STRING Hello from HID Lab
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
STRING echo HID Lab Connected
ENTER
`,
};

function loadTemplate(name) {
    refs.editor.value = TEMPLATES[name] || '';
    refs.payloadName.value = `${name}.ducky`;
    currentPayload = '';
    setBusyState(running);
}

function installHandlers() {
    $('btnAuth').onclick = unlockOrSetup;
    $('btnLogout').onclick = logout;
    $('btnSave').onclick = savePayload;
    $('btnRun').onclick = runPayload;
    $('btnStop').onclick = stopExecution;
    $('btnDelete').onclick = deletePayload;
    $('btnLive').onclick = runLive;
    $('btnAutorun').onclick = setAutorun;
    $('btnSaveUsbId').onclick = saveUsbId;
    $('btnTemplate1').onclick = () => loadTemplate('notepad');
    $('btnTemplate2').onclick = () => loadTemplate('browser');
    $('btnTemplate3').onclick = () => loadTemplate('terminal');
    $('btnNew').onclick = () => {
        currentPayload = '';
        refs.payloadName.value = '';
        refs.editor.value = '';
        loadPayloads();
        setBusyState(running);
    };

    refs.adminToken.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            unlockOrSetup();
        }
    });

    refs.editor.addEventListener('keydown', (e) => {
        if (e.key === 'Tab') {
            e.preventDefault();
            const start = refs.editor.selectionStart;
            const end = refs.editor.selectionEnd;
            refs.editor.value =
                refs.editor.value.substring(0, start) +
                '    ' +
                refs.editor.value.substring(end);
            refs.editor.selectionStart = refs.editor.selectionEnd = start + 4;
        }
    });

    document.addEventListener('keydown', (e) => {
        if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 's') {
            e.preventDefault();
            if (authenticated) savePayload();
        }
    });
}

async function boot() {
    installHandlers();
    setBusyState(false);

    try {
        const status = await refreshStatus();
        if (status.authenticated) {
            await loadSettings();
            await loadPayloads();
        }
    } catch (e) {
        refs.statusText.textContent = 'Offline';
        refs.statusDot.className = 'status-indicator error';
        showAuthPanel();
    }

    statusTimer = setInterval(async () => {
        try {
            const status = await refreshStatus();
            if (status.authenticated) {
                await loadPayloads();
            }
        } catch (e) {
            refs.statusText.textContent = 'Offline';
            refs.statusDot.className = 'status-indicator error';
        }
    }, 2500);
}

document.addEventListener('DOMContentLoaded', boot);
