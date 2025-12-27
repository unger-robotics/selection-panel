/* ==========================================================================
   Schematic JavaScript v7.3
   Stand: 2025-12-27
   Funktionen: Tab-Navigation, LED-Animation
   Farbschema: CSS-gesteuert (Arduino Teal + Raspberry Pi)
   NEU: Tabs für vollständige Pinout-Tabellen
   ========================================================================== */

document.addEventListener('DOMContentLoaded', () => {
    initTabs();
    initLEDAnimation();
});

/* --------------------------------------------------------------------------
   Tab Navigation
   -------------------------------------------------------------------------- */
function initTabs() {
    const tabs = document.querySelectorAll('.tab');
    const views = document.querySelectorAll('.view');
    
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const viewId = tab.dataset.view;
            
            // Deactivate all
            tabs.forEach(t => t.classList.remove('active'));
            views.forEach(v => v.classList.remove('active'));
            
            // Activate selected
            tab.classList.add('active');
            document.getElementById(viewId)?.classList.add('active');
        });
    });
}

/* --------------------------------------------------------------------------
   LED Animation (Prototype View)
   One-hot Verhalten: Immer nur eine LED aktiv (wie im echten System)
   -------------------------------------------------------------------------- */
function initLEDAnimation() {
    const leds = document.querySelectorAll('.led');
    if (leds.length === 0) return;
    
    let currentLED = 0;
    
    setInterval(() => {
        // Clear all (One-hot: nur eine LED aktiv)
        leds.forEach(led => led.classList.remove('active'));
        
        // Activate current
        leds[currentLED]?.classList.add('active');
        
        // Next LED
        currentLED = (currentLED + 1) % leds.length;
    }, 500);
}

/* --------------------------------------------------------------------------
   Utility: Copy Pin Table to Clipboard
   -------------------------------------------------------------------------- */
function copyPinTable() {
    const table = document.querySelector('.card-primary .pin-table');
    if (!table) return;
    
    let text = 'ESP32-S3 XIAO Pinbelegung\n';
    text += '─'.repeat(30) + '\n';
    
    table.querySelectorAll('tbody tr').forEach(row => {
        const cells = row.querySelectorAll('td');
        text += `${cells[0].textContent.padEnd(4)} ${cells[1].textContent.padEnd(8)} ${cells[2].textContent}\n`;
    });
    
    navigator.clipboard.writeText(text).then(() => {
        console.log('Pin table copied to clipboard');
    });
}

/* --------------------------------------------------------------------------
   Debug: Log Current State
   -------------------------------------------------------------------------- */
function logState() {
    const activeTab = document.querySelector('.tab.active');
    const activeView = document.querySelector('.view.active');
    const activeLED = document.querySelector('.led.active');
    
    console.log('Schematic State:', {
        activeTab: activeTab?.dataset.view,
        activeView: activeView?.id,
        ledCount: document.querySelectorAll('.led').length,
        activeLED: activeLED?.classList.contains('active') ? 
            Array.from(document.querySelectorAll('.led')).indexOf(activeLED) : -1
    });
}

/* --------------------------------------------------------------------------
   Keyboard Shortcuts
   -------------------------------------------------------------------------- */
document.addEventListener('keydown', (e) => {
    // 1-5 für Tab-Wechsel
    if (e.key === '1') document.querySelector('[data-view="prototype"]')?.click();
    if (e.key === '2') document.querySelector('[data-view="full"]')?.click();
    if (e.key === '3') document.querySelector('[data-view="timing"]')?.click();
    if (e.key === '4') document.querySelector('[data-view="pinout-595"]')?.click();
    if (e.key === '5') document.querySelector('[data-view="pinout-4021"]')?.click();
    
    // D für Debug
    if (e.key === 'd' && e.ctrlKey) {
        e.preventDefault();
        logState();
    }
});
