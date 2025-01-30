
let selectedFile = null;

function addFile(fileName) {
    const fileList = document.getElementById('file-list');

    const fileItem = document.createElement('div');
    fileItem.className = 'file-item';
    fileItem.onclick = () => selectFile(fileItem, fileName);

    const fileNameSpan = document.createElement('span');
    fileNameSpan.textContent = fileName;

    fileItem.appendChild(fileNameSpan);
    fileList.appendChild(fileItem);
}

function selectFile(element, fileName) {
    selectedFile = fileName;

    const fileItems = document.querySelectorAll('.file-item');
    fileItems.forEach(item => item.classList.remove('selected'));

    element.classList.add('selected');
    console.log(`Selected file: ${selectedFile}`);
}

function removeSelectedFile() {
    const fileList = document.getElementById('file-list');
    const selectedElement = Array.from(fileList.children).find(item =>
        item.classList.contains('selected')
    );

    if (selectedElement) {
        fileList.removeChild(selectedElement);
        selectedFile = null;
    }
}

function deleteFile() {
    removeSelectedFile();
}

function downloadFile() {
    if (!selectedFile) {
        alert('Please select a file to download.');
        return;
    }
    alert(`Downloading: ${selectedFile}`);
}

async function configureFPGA(fileName) {
    const status = document.getElementById('status');
    status.innerText = "Starting FPGA configuration...";
    
    if (fileName.includes("(Active):")) {
        fileName = fileName.split("(Active):")[1];
        console.log(fileName); 
    } else {
        console.log("The string does not contain '(Active):'");
    }
    const payload = JSON.stringify({ file_name: fileName });

    try {
        const response = await fetch('/fpga_config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: payload,
        });

        if (response.ok) {
            const result = await response.text();
            console.log("FPGA Configuration Response:", result);
            status.innerText = "FPGA configured successfully!";
            fetchFileList();
        } else {
            console.error("FPGA Configuration Failed:", response.statusText);
            status.innerText = `Configuration failed: ${response.statusText}`;
        }
    } catch (error) {
        console.error("Error configuring FPGA:", error);
        status.innerText = `Error: ${error.message}`;
    }
}

document.addEventListener('DOMContentLoaded', () => {
    fetchFileList();
    addFile('Fetch Remote File to List Here!');
    document.getElementById('configure_btn').addEventListener('click', () => configureFPGA(selectedFile));
}); 