const tabs = document.querySelectorAll('.tab');
const panels = document.querySelectorAll('.tab-panel');

for (const tab of tabs) {
  tab.addEventListener('click', () => {
    tabs.forEach((t) => t.classList.remove('active'));
    panels.forEach((p) => p.classList.remove('active'));
    tab.classList.add('active');
    const panel = document.getElementById(tab.dataset.tab);
    if (panel) panel.classList.add('active');
  });
}

const objectPresets = {
  microstrip: {
    name: 'MicrostripLine', x: '0', y: '0', z: '0', w: '1.20', h: '0.035', l: '28', material: 'Copper'
  },
  gnd: {
    name: 'GroundPlane', x: '0', y: '-1.0', z: '0', w: '40', h: '0.035', l: '80', material: 'Copper'
  },
  substrate: {
    name: 'SubstrateBlock', x: '0', y: '-0.8', z: '0', w: '40', h: '0.80', l: '80', material: 'FR4'
  },
  airbox: {
    name: 'AirBox', x: '0', y: '0', z: '0', w: '100', h: '50', l: '100', material: 'Air'
  },
  port1: {
    name: 'Port_P1_Face', x: '0', y: '0', z: '0', w: '1.20', h: '0.80', l: '0.50'
  },
  setup: {
    name: 'Setup_FreqSweep', x: '-', y: '-', z: '-', w: '-', h: '-', l: '-'
  }
};

const fields = {
  name: document.getElementById('obj-name'),
  x: document.getElementById('obj-x'),
  y: document.getElementById('obj-y'),
  z: document.getElementById('obj-z'),
  w: document.getElementById('obj-w'),
  h: document.getElementById('obj-h'),
  l: document.getElementById('obj-l')
};

const hideBtn = document.getElementById('hide-btn');
const showBtn = document.getElementById('show-btn');
const selectionHud = document.getElementById('selection-hud');
const viewport = document.querySelector('.viewport');
const popup = document.getElementById('materials-popup');
const popupClose = document.getElementById('popup-close');
const popupApply = document.getElementById('popup-apply');
const popupMaterial = document.getElementById('popup-material');
const popupSelectedList = document.getElementById('popup-selected-list');

const treeNodesByTarget = {};
for (const node of document.querySelectorAll('.node[data-target]')) {
  treeNodesByTarget[node.dataset.target] = node;
}

const sceneButtonsByTarget = {};
for (const button of document.querySelectorAll('.scene-object[data-target]')) {
  sceneButtonsByTarget[button.dataset.target] = button;
}

const materialByObject = {
  microstrip: 'Copper',
  gnd: 'Copper',
  substrate: 'FR4',
  airbox: 'Air'
};

const visibilityByObject = {
  microstrip: true,
  gnd: true,
  substrate: true,
  airbox: true
};

const selectedObjects = new Set(['microstrip']);
let activeTarget = 'microstrip';
let selectedNode = treeNodesByTarget.microstrip;

function setInspectorFromTarget(target) {
  const preset = objectPresets[target];
  if (!preset) return;
  fields.name.value = preset.name;
  fields.x.value = preset.x;
  fields.y.value = preset.y;
  fields.z.value = preset.z;
  fields.w.value = preset.w;
  fields.h.value = preset.h;
  fields.l.value = preset.l;
}

function updateHideShowButtons() {
  if (selectedObjects.size !== 1) {
    hideBtn.disabled = true;
    showBtn.disabled = true;
    return;
  }
  const target = Array.from(selectedObjects)[0];
  const isVisible = visibilityByObject[target] !== false;
  hideBtn.disabled = !isVisible;
  showBtn.disabled = isVisible;
}

function refreshVisibilityStyles() {
  for (const [target, node] of Object.entries(treeNodesByTarget)) {
    if (!node.dataset.object) continue;
    const isVisible = visibilityByObject[target] !== false;
    node.classList.toggle('hidden-object', !isVisible);
  }
  for (const [target, button] of Object.entries(sceneButtonsByTarget)) {
    const isVisible = visibilityByObject[target] !== false;
    button.classList.toggle('hidden-object', !isVisible);
  }
}

function syncSelectionStyles() {
  for (const [target, node] of Object.entries(treeNodesByTarget)) {
    node.classList.remove('active', 'multi-selected');
    if (!node.dataset.object) continue;
    if (selectedObjects.has(target)) {
      if (target === activeTarget) {
        node.classList.add('active');
      } else {
        node.classList.add('multi-selected');
      }
    }
  }

  for (const [target, button] of Object.entries(sceneButtonsByTarget)) {
    button.classList.toggle('selected', selectedObjects.has(target));
  }
}

function updateSelectionHud() {
  if (selectedObjects.size === 0) {
    selectionHud.textContent = 'Selection: none';
    return;
  }

  if (selectedObjects.size > 1) {
    selectionHud.textContent = `Selection: ${selectedObjects.size} objects`;
    return;
  }

  const target = Array.from(selectedObjects)[0];
  const preset = objectPresets[target];
  const isVisible = visibilityByObject[target] !== false;
  selectionHud.textContent = `Selection: ${preset.name} (${isVisible ? 'Visible' : 'Hidden'})`;
}

function selectObject(target, additive) {
  if (!objectPresets[target]) return;
  if (!additive) {
    selectedObjects.clear();
    selectedObjects.add(target);
    activeTarget = target;
  } else {
    if (selectedObjects.has(target) && selectedObjects.size > 1) {
      selectedObjects.delete(target);
    } else {
      selectedObjects.add(target);
      activeTarget = target;
    }
  }
  selectedNode = treeNodesByTarget[target];
  setInspectorFromTarget(activeTarget);
  syncSelectionStyles();
  refreshVisibilityStyles();
  updateHideShowButtons();
  updateSelectionHud();
}

function regroupTreeByMaterial() {
  const tree = document.querySelector('.tree');
  const materialOrder = ['Copper', 'FR4', 'Air'];
  for (const material of materialOrder) {
    const label = tree.querySelector(`.group-label[data-material-group="${material}"]`);
    if (!label) continue;
    const nodes = Object.keys(materialByObject)
      .filter((target) => materialByObject[target] === material)
      .map((target) => treeNodesByTarget[target])
      .filter(Boolean);
    let insertAfter = label;
    for (const node of nodes) {
      if (node.previousElementSibling !== insertAfter) {
        insertAfter.insertAdjacentElement('afterend', node);
      }
      insertAfter = node;
    }
  }
}

function openMaterialPopup(x, y) {
  if (selectedObjects.size === 0) return;
  popup.classList.remove('hidden');
  popup.style.left = `${Math.max(20, Math.min(window.innerWidth - 360, x))}px`;
  popup.style.top = `${Math.max(20, Math.min(window.innerHeight - 280, y))}px`;

  popupSelectedList.innerHTML = '';
  const selected = Array.from(selectedObjects);
  const firstMaterial = materialByObject[selected[0]];
  const sameMaterial = selected.every((target) => materialByObject[target] === firstMaterial);
  popupMaterial.value = sameMaterial ? firstMaterial : 'Copper';

  for (const target of selected) {
    const li = document.createElement('li');
    li.textContent = `${objectPresets[target].name} (${materialByObject[target]})`;
    popupSelectedList.appendChild(li);
  }
}

function closeMaterialPopup() {
  popup.classList.add('hidden');
}

for (const node of document.querySelectorAll('.node[data-target]')) {
  node.addEventListener('click', (event) => {
    const target = node.dataset.target;
    if (node.dataset.object === 'true') {
      selectObject(target, event.ctrlKey || event.metaKey);
      return;
    }
    document.querySelectorAll('.node').forEach((n) => n.classList.remove('active', 'multi-selected'));
    node.classList.add('active');
    selectedObjects.clear();
    activeTarget = target;
    selectedNode = node;
    setInspectorFromTarget(target);
    syncSelectionStyles();
    updateHideShowButtons();
    updateSelectionHud();
  });
}

for (const button of document.querySelectorAll('.scene-object[data-target]')) {
  button.addEventListener('click', (event) => {
    event.stopPropagation();
    selectObject(button.dataset.target, event.ctrlKey || event.metaKey);
  });
}

viewport.addEventListener('contextmenu', (event) => {
  if (selectedObjects.size === 0) return;
  event.preventDefault();
  openMaterialPopup(event.clientX, event.clientY);
});

popupApply.addEventListener('click', () => {
  const selectedMaterial = popupMaterial.value;
  for (const target of selectedObjects) {
    materialByObject[target] = selectedMaterial;
  }
  regroupTreeByMaterial();
  closeMaterialPopup();
});

popupClose.addEventListener('click', closeMaterialPopup);

hideBtn.addEventListener('click', () => {
  if (selectedObjects.size !== 1) return;
  const target = Array.from(selectedObjects)[0];
  visibilityByObject[target] = false;
  refreshVisibilityStyles();
  updateHideShowButtons();
  updateSelectionHud();
});

showBtn.addEventListener('click', () => {
  if (selectedObjects.size !== 1) return;
  const target = Array.from(selectedObjects)[0];
  visibilityByObject[target] = true;
  refreshVisibilityStyles();
  updateHideShowButtons();
  updateSelectionHud();
});

window.addEventListener('click', (event) => {
  if (!popup.classList.contains('hidden') && !popup.contains(event.target)) {
    closeMaterialPopup();
  }
});

regroupTreeByMaterial();
syncSelectionStyles();
refreshVisibilityStyles();
updateHideShowButtons();
updateSelectionHud();
setInspectorFromTarget(activeTarget);

const portName = document.getElementById('port-name');
const portZ = document.getElementById('port-z');
const portMessage = document.getElementById('port-message');

function validatePort() {
  const name = (portName.value || '').trim();
  const z = Number(portZ.value);

  if (!name) {
    portMessage.textContent = 'Blocking error: Port name is required.';
    portMessage.className = 'error';
    return;
  }

  if (!Number.isFinite(z) || z <= 0) {
    portMessage.textContent = 'Blocking error: Impedance must be a positive number.';
    portMessage.className = 'error';
    return;
  }

  if (z < 20 || z > 120) {
    portMessage.textContent = 'Warning: impedance is outside typical SI range (20-120 ohm).';
    portMessage.className = 'warn';
    return;
  }

  portMessage.textContent = 'Port definition valid.';
  portMessage.className = 'ok';
}

portName.addEventListener('input', validatePort);
portZ.addEventListener('input', validatePort);
validatePort();
