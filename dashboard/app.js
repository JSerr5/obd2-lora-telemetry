// Variables Globales
const ORION_URL = '/v2/entities/urn:ngsi-ld:Motorcycle:CB300F_Node01';
const HEADERS = {};

const CONFIG = {
    TEMP: 65,
    RPM: 3000,
    VEL: 3,
    TIEMPO: 20
};

// Utilidades UI
function logConsole(msg) {
    const cons = document.getElementById('console-output');
    cons.innerHTML += `> ${msg}<br>`;
    cons.scrollTop = cons.scrollHeight;
}

document.getElementById('firmware-file').addEventListener('change', function(e) {
    if(e.target.files.length > 0) {
        const nameDiv = document.getElementById('file-name');
        nameDiv.innerText = e.target.files[0].name;
        nameDiv.classList.remove('d-none');
        logConsole(`Archivo cargado en memoria: ${e.target.files[0].name}`);
    }
});

// Configuración Base de Chart.js
Chart.defaults.font.family = "'Outfit', sans-serif";
// Color de grid por defecto (se actualiza con el tema)
let chartGridColor = 'rgba(255, 255, 255, 0.05)';
Chart.defaults.color = 'rgba(150, 150, 150, 0.8)';

function createChart(id, color, gradientStart) {
    const ctx = document.getElementById(id).getContext('2d');
    
    // Crear gradiente para el área bajo la curva
    let gradient = ctx.createLinearGradient(0, 0, 0, 100);
    gradient.addColorStop(0, gradientStart);
    gradient.addColorStop(1, 'rgba(0,0,0,0)');

    return new Chart(ctx, {
        type: 'line',
        data: {
            labels: Array(15).fill(''),
            datasets: [{
                data: Array(15).fill(0),
                borderColor: color,
                borderWidth: 3,
                backgroundColor: gradient,
                fill: true,
                pointRadius: 0,
                tension: 0.4 // Curvas suaves
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: { duration: 300 }, // Transiciones rápidas pero suaves
            plugins: { legend: false, tooltip: { enabled: false } },
            scales: {
                x: { 
                    display: true, 
                    grid: { color: () => chartGridColor, drawBorder: false },
                    border: { display: false }
                },
                y: { 
                    display: true, 
                    min: 0,
                    grid: { color: () => chartGridColor, drawBorder: false },
                    border: { display: false }
                }
            }
        }
    });
}

// Inicializar Gráficos
const chartRpm = createChart('chart-rpm', '#3b82f6', 'rgba(59, 130, 246, 0.3)');
const chartVel = createChart('chart-vel', '#10b981', 'rgba(16, 185, 129, 0.3)');

function updateChart(chart, value) {
    chart.data.datasets[0].data.push(value);
    chart.data.datasets[0].data.shift();
    chart.update();
}

// Fetch Data Engine
async function fetchData() {
    try {
        const res = await fetch(ORION_URL, { headers: HEADERS });
        if(!res.ok) throw new Error("Network response was not ok");
        
        const data = await res.json();

        const rpm = data.rpm ? parseFloat(data.rpm.value) : 0;
        const temp = data.temperature ? parseFloat(data.temperature.value) : 0;
        const vel = data.speed ? parseFloat(data.speed.value) : 0;
        const dist = 0; // No enviado aún por el gateway

        const tiempo = data.time ? parseFloat(data.time.value) : 0;

        // Actualizar UI - Valores Numéricos
        document.getElementById('rpm-val').innerText = rpm.toFixed(0);
        document.getElementById('vel-val').innerText = vel.toFixed(0);
        document.getElementById('temp-val').innerText = temp.toFixed(0);
        document.getElementById('dist-val').innerText = dist.toFixed(2);

        // Barra de Temperatura (máx 150°C aprox para llenado visual)
        const tempPercent = Math.min((temp / 150) * 100, 100);
        document.getElementById('temp-bar').style.width = `${tempPercent}%`;

        // Lógica Semáforo
        const st = document.getElementById('status-val');
        const mt = document.getElementById('motivo-val');
        const lRed = document.getElementById('light-red');
        const lYellow = document.getElementById('light-yellow');
        const lGreen = document.getElementById('light-green');
        
        st.classList.remove('blink');
        lRed.classList.remove('active-red');
        lYellow.classList.remove('active-yellow', 'blink');
        lGreen.classList.remove('active-green');

        if (rpm <= 0) {
            st.innerText = "INACTIVO";
            st.className = "fw-bold mb-1 status-inactivo";
            mt.innerText = "Motor apagado";
        } else if (temp >= CONFIG.TEMP && tiempo >= CONFIG.TIEMPO) {
            st.innerText = "ÓPTIMO";
            st.className = "fw-bold mb-1 status-optimo";
            mt.innerText = "Sistema estabilizado";
            lGreen.classList.add('active-green');
        } else if (vel > CONFIG.VEL || rpm > CONFIG.RPM) {
            st.innerText = "ADVERTENCIA";
            st.className = "fw-bold mb-1 status-advertencia blink";
            mt.innerText = "Conducción en frío";
            lRed.classList.add('active-red'); // Doble advertencia (Rojo encendido)
            lYellow.classList.add('active-yellow', 'blink'); // Amarillo parpadeante
        } else {
            st.innerText = "CALENTANDO";
            st.className = "fw-bold mb-1 status-calentando";
            mt.innerText = "Alcanzando temperatura";
            lRed.classList.add('active-red');
        }

        // Actualizar Badge de Conexión
        const badge = document.getElementById('status-badge');
        badge.className = "badge rounded-pill bg-success px-3 py-2 text-white fw-bold shadow-sm";
        badge.innerHTML = `<i class="fa-solid fa-link me-1"></i> Conectado`;
        
        document.getElementById('last-update').innerText = `Última trama: ${new Date().toLocaleTimeString()}`;

        // Actualizar Gráficos Reales
        updateChart(chartRpm, rpm);
        updateChart(chartVel, vel);

    } catch (e) {
        console.error(e);
        const badge = document.getElementById('status-badge');
        badge.className = "badge rounded-pill bg-danger px-3 py-2 text-white fw-bold shadow-sm blink";
        badge.innerHTML = `<i class="fa-solid fa-triangle-exclamation me-1"></i> Error API`;
    }
}

// OTA Logic
async function startUpdate() {
    const fileInput = document.getElementById('firmware-file');
    const file = fileInput.files[0];
    
    if (!file) {
        alert("Por favor selecciona el archivo de firmware .bin primero.");
        return;
    }

    const btn = document.getElementById('btn-update');
    const bar = document.getElementById('firmware-bar');
    const progressText = document.getElementById('progress-percent');

    btn.disabled = true;
    btn.innerHTML = `<i class="fa-solid fa-circle-notch fa-spin me-2"></i>Preparando...`;
    
    logConsole("Iniciando secuencia OTA...");

    let fd = new FormData();
    fd.append("firmware", file);

    try {
        logConsole("Enviando bloque binario al servidor temporal...");
        bar.style.width = "40%";
        progressText.innerText = "40%";
        
        await fetch("http://10.162.40.214:5000/upload", { method: "POST", body: fd });
        
        logConsole("Archivo recibido por servidor.");
        bar.style.width = "70%";
        progressText.innerText = "70%";

        logConsole("Notificando mediante MQTT a la Moto para forzar flasheo...");
        await fetch("http://10.162.40.214:5000/trigger", { method: "POST" });
        
        bar.className = "progress-bar bg-success";
        bar.style.width = "100%";
        progressText.innerText = "100%";
        
        logConsole("¡Señal OTA enviada con éxito! La moto comenzará el reinicio.");
        btn.innerHTML = `<i class="fa-solid fa-check me-2"></i>OTA Enviada`;

        // Restaurar estado para permitir futuros OTA sin recargar la página
        setTimeout(() => {
            btn.disabled = false;
            btn.innerHTML = `<i class="fa-solid fa-rocket me-2"></i>Lanzar OTA a la Moto`;
            bar.style.width = "0%";
            progressText.innerText = "0%";
            bar.className = "progress-bar progress-bar-striped progress-bar-animated bg-primary";
            fileInput.value = "";
            const fileNameDiv = document.getElementById('file-name');
            if (fileNameDiv) fileNameDiv.classList.add('d-none');
        }, 4000);

    } catch(e) {
        logConsole("ERROR FATAL: " + e.message);
        bar.className = "progress-bar bg-danger";
        btn.innerHTML = `<i class="fa-solid fa-rotate-right me-2"></i>Reintentar`;
        btn.disabled = false;
    }
}

// Lógica del Tema (Oscuro/Claro)
const themeToggle = document.getElementById('theme-toggle');
const htmlTag = document.documentElement;
const themeIcon = themeToggle.querySelector('i');

function setTheme(theme) {
    htmlTag.setAttribute('data-bs-theme', theme);
    localStorage.setItem('theme', theme);
    
    if(theme === 'light') {
        themeIcon.className = 'fa-solid fa-sun fs-5 text-warning';
        chartGridColor = 'rgba(0, 0, 0, 0.05)';
    } else {
        themeIcon.className = 'fa-solid fa-moon fs-5';
        chartGridColor = 'rgba(255, 255, 255, 0.05)';
    }
    
    // Forzar redibujado de las grids de los gráficos
    if(chartRpm) chartRpm.update('none');
    if(chartVel) chartVel.update('none');
}

// Inicializar tema al cargar
const savedTheme = localStorage.getItem('theme') || 'dark';
setTheme(savedTheme);

themeToggle.addEventListener('click', () => {
    const currentTheme = htmlTag.getAttribute('data-bs-theme');
    setTheme(currentTheme === 'dark' ? 'light' : 'dark');
});

// Bucle del Motor de Adquisición de Datos
setInterval(fetchData, 1000); // 1 segundo para tiempo real estricto
fetchData();
