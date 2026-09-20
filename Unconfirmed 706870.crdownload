/* FlowShield Offline Earth Map
   Self-contained map engine: no CDN, tiles, localhost, API key or external requests.
*/
(function(){
  const esc=s=>String(s).replace(/[&<>]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;'}[c]));
  const NS='http://www.w3.org/2000/svg';
  const svgEl=(tag,attrs={})=>{const e=document.createElementNS(NS,tag);Object.entries(attrs).forEach(([k,v])=>e.setAttribute(k,v));return e};

  class MapMock{
    constructor(id){
      this.container=document.getElementById(id);this.handlers={};this.layers=[];this.center={lat:12.9716,lng:77.5946};this.zoom=11;
      this.svg=svgEl('svg',{class:'earth-map-svg',width:'100%',height:'100%',preserveAspectRatio:'none'});
      this.container.innerHTML='';this.container.appendChild(this.svg);
      this.bg=svgEl('g');this.grid=svgEl('g');this.labels=svgEl('g');this.layerRoot=svgEl('g');this.svg.append(this.bg,this.grid,this.labels,this.layerRoot);
      this.popup=document.createElement('div');this.popup.className='offline-map-popup';this.popup.style.display='none';this.container.appendChild(this.popup);
      this.renderBackground();
      this.svg.addEventListener('click',e=>{if(e.target.closest('.map-feature'))return;const p=this.pointFromEvent(e);this.fire('click',{latlng:p})});
      this.svg.addEventListener('wheel',e=>{e.preventDefault();this.zoom=Math.max(8,Math.min(15,this.zoom+(e.deltaY<0?1:-1)));this.render()},{passive:false});
      this.drag=null;
      this.svg.addEventListener('pointerdown',e=>{this.drag={x:e.clientX,y:e.clientY,center:{...this.center}};this.svg.setPointerCapture(e.pointerId)});
      this.svg.addEventListener('pointermove',e=>{if(!this.drag)return;const w=this.svg.clientWidth,h=this.svg.clientHeight,span=this.span();const dx=(e.clientX-this.drag.x)/w*span.lng,dy=(e.clientY-this.drag.y)/h*span.lat;this.center.lng=this.drag.center.lng-dx;this.center.lat=this.drag.center.lat+dy;this.render()});
      this.svg.addEventListener('pointerup',()=>this.drag=null);this.svg.addEventListener('pointercancel',()=>this.drag=null);
      new ResizeObserver(()=>this.render()).observe(this.container);
    }
    span(){const h=0.11*Math.pow(11/this.zoom,1);return {lat:h,lng:h*1.38*(this.svg.clientWidth/Math.max(1,this.svg.clientHeight))};}
    project(lat,lng){const w=this.svg.clientWidth||800,h=this.svg.clientHeight||500,s=this.span();return {x:w/2+(lng-this.center.lng)/s.lng*w,y:h/2-(lat-this.center.lat)/s.lat*h};}
    unproject(x,y){const w=this.svg.clientWidth||800,h=this.svg.clientHeight||500,s=this.span();return {lat:this.center.lat-(y-h/2)/h*s.lat,lng:this.center.lng+(x-w/2)/w*s.lng};}
    pointFromEvent(e){const r=this.svg.getBoundingClientRect();return this.unproject(e.clientX-r.left,e.clientY-r.top)}
    setView(c,z){this.center={lat:c[0],lng:c[1]};if(z)this.zoom=z;this.render();return this}
    on(ev,fn){(this.handlers[ev]??=[]).push(fn);return this}
    fire(ev,data){(this.handlers[ev]||[]).forEach(fn=>fn(data))}
    addLayer(l){if(!this.layers.includes(l)){this.layers.push(l);l._map=this;l.redraw?.()}return this}
    removeLayer(l){const i=this.layers.indexOf(l);if(i>=0)this.layers.splice(i,1);l._map=null;l.remove?.();return this}
    render(){this.renderBackground();this.layers.forEach(l=>l.redraw?.())}
    renderBackground(){
      while(this.bg.firstChild)this.bg.firstChild.remove();while(this.grid.firstChild)this.grid.firstChild.remove();while(this.labels.firstChild)this.labels.firstChild.remove();
      const w=this.svg.clientWidth||800,h=this.svg.clientHeight||500;
      this.bg.appendChild(svgEl('rect',{x:0,y:0,width:w,height:h,fill:'#071b18'}));
      // Ocean depth rings / subtle earth texture
      for(let i=0;i<7;i++)this.bg.appendChild(svgEl('ellipse',{cx:w*(.18+i*.13),cy:h*(.2+i*.08),rx:w*(.22+i*.03),ry:h*(.32+i*.02),fill:'none',stroke:'#123a32','stroke-width':'1',opacity:.55}));
      // Stylised landmass around Bengaluru / South India
      const pts=[
        [0.35,0.08],[0.46,0.10],[0.57,0.18],[0.63,0.31],[0.60,0.42],[0.55,0.50],[0.56,0.61],[0.51,0.73],[0.43,0.88],[0.36,0.77],[0.31,0.64],[0.25,0.55],[0.24,0.42],[0.28,0.29],[0.31,0.17]
      ].map(([x,y])=>`${x*w},${y*h}`).join(' ');
      this.bg.appendChild(svgEl('polygon',{points:pts,fill:'#17382e',stroke:'#2c6a55','stroke-width':'2',opacity:.96}));
      // Karnataka-ish inner terrain bands
      for(let i=0;i<5;i++){
        const y=h*(.26+i*.11);this.bg.appendChild(svgEl('path',{d:`M ${w*.27} ${y} Q ${w*.42} ${y-28} ${w*.59} ${y+8}`,fill:'none',stroke:'#275342','stroke-width':1,opacity:.65}));
      }
      // Water bodies
      const lakes=[['Bellandur',.58,.67,.075,.028],['Varthur',.68,.63,.05,.022],['Ulsoor',.49,.39,.025,.018],['Nagavara',.44,.27,.028,.015]];
      lakes.forEach(([name,x,y,rx,ry])=>{const e=svgEl('ellipse',{cx:x*w,cy:y*h,rx:rx*w,ry:ry*h,fill:'#0d5361',stroke:'#4b9ca5','stroke-width':'1',opacity:.9});e.dataset.label=name;this.bg.appendChild(e);const t=svgEl('text',{x:x*w+4,y:y*h-3,fill:'#70b9bf','font-size':'9','font-family':'system-ui'});t.textContent=name;this.bg.appendChild(t)});
      // Roads / corridors
      const roads=[
        [[.30,.65],[.39,.57],[.49,.48],[.58,.36],[.69,.25]],
        [[.27,.43],[.39,.45],[.51,.50],[.65,.54],[.76,.58]],
        [[.33,.23],[.40,.34],[.48,.48],[.55,.62],[.59,.78]],
        [[.26,.55],[.38,.60],[.52,.65],[.67,.72]]
      ];
      roads.forEach((r,i)=>{const d=r.map(([x,y],j)=>(j?'L':'M')+` ${x*w} ${y*h}`).join(' ');this.bg.appendChild(svgEl('path',{d,fill:'none',stroke:i===1?'#d0b86d':'#8f9b79','stroke-width':i===1?'3':'2',opacity:.55}));});
      // City label and compass
      const city=svgEl('text',{x:w*.50,y:h*.52,fill:'#d9f3e7','font-size':'15','font-weight':'800','font-family':'system-ui','text-anchor':'middle'});city.textContent='BENGALURU FLOOD MODEL';this.labels.appendChild(city);
      const sub=svgEl('text',{x:w*.50,y:h*.55,fill:'#79a99a','font-size':'9','font-family':'system-ui','text-anchor':'middle'});sub.textContent='Interactive offline earth map • click to select sensor point';this.labels.appendChild(sub);
      const north=svgEl('text',{x:w-25,y:34,fill:'#b7d7ca','font-size':'12','font-weight':'800','text-anchor':'middle'});north.textContent='N';this.labels.appendChild(north);
      this.labels.appendChild(svgEl('path',{d:`M ${w-25} 43 L ${w-31} 57 L ${w-19} 57 Z`,fill:'#54d6a0'}));
      // Lat/lng guide lines
      const s=this.span();for(let i=-4;i<=4;i++){const lat=this.center.lat+i*s.lat/4;const p=this.project(lat,this.center.lng);this.grid.appendChild(svgEl('line',{x1:0,x2:w,y1:p.y,y2:p.y,stroke:'#1c493e','stroke-width':1,opacity:.45}));}
      for(let i=-5;i<=5;i++){const lng=this.center.lng+i*s.lng/5;const p=this.project(this.center.lat,lng);this.grid.appendChild(svgEl('line',{x1:p.x,x2:p.x,y1:0,y2:h,stroke:'#1c493e','stroke-width':1,opacity:.35}));}
    }
  }

  class LayerGroup{constructor(){this.layers=[]}addLayer(l){this.layers.push(l);return this}addTo(map){map.addLayer(this);this._map=map;this.layers.forEach(l=>{l._group=this;l._map=map;l.redraw?.()});return this}eachLayer(fn){this.layers.forEach(fn)}removeLayer(l){this.layers=this.layers.filter(x=>x!==l)}redraw(){this.layers.forEach(l=>l.redraw?.())}remove(){this.layers.forEach(l=>l.remove?.());this.layers=[]}}
  class Rectangle{
    constructor(bounds,opt={}){this.bounds=bounds;this.opt={...opt};this.handlers={};this.el=svgEl('rect',{class:'map-feature'});this.title=svgEl('title');this.el.appendChild(this.title);this.el.addEventListener('click',e=>{e.stopPropagation();(this.handlers.click||[]).forEach(fn=>fn({originalEvent:e}))});}
    bindTooltip(html){this.tooltip=html;this.title.textContent=strip(html);return this}
    setTooltipContent(html){this.tooltip=html;this.title.textContent=strip(html);return this}
    on(ev,fn){(this.handlers[ev]??=[]).push(fn);return this}
    setStyle(s){Object.assign(this.opt,s);this.redraw();return this}
    addTo(target){if(target instanceof LayerGroup)return target.addLayer(this);target.addLayer(this);return this}
    redraw(){if(!this._map)return;const [a,b]=this.bounds;const p1=this._map.project(a[0],a[1]),p2=this._map.project(b[0],b[1]);const x=Math.min(p1.x,p2.x),y=Math.min(p1.y,p2.y),w=Math.abs(p2.x-p1.x),h=Math.abs(p2.y-p1.y);this.el.setAttribute('x',x);this.el.setAttribute('y',y);this.el.setAttribute('width',w);this.el.setAttribute('height',h);this.el.setAttribute('fill',this.opt.fillColor||'#54d6a0');this.el.setAttribute('fill-opacity',this.opt.fillOpacity??.28);this.el.setAttribute('stroke',this.opt.color||'#1c3b31');this.el.setAttribute('stroke-width',this.opt.weight||1);if(!this.el.parentNode)this._map.layerRoot.appendChild(this.el)}
    remove(){this.el.remove()}
  }
  class Marker{
    constructor(latlng,opt={}){this.latlng=latlng;this.opt=opt;this.handlers={};this.g=svgEl('g',{class:'map-feature'});this.r=svgEl('circle',{r:8,fill:'#54d6a0',stroke:'#05120d','stroke-width':3});this.p=svgEl('circle',{r:15,fill:'none',stroke:'#54d6a0','stroke-width':2,opacity:.35});this.g.append(this.p,this.r);this.g.addEventListener('click',e=>e.stopPropagation());}
    addTo(map){this._map=map;map.addLayer(this);return this}
    bindPopup(html){this.popupHtml=html;return this}
    openPopup(){if(this._map){this._map.popup.innerHTML=this.popupHtml||'';this._map.popup.style.display='block';this.positionPopup();}return this}
    positionPopup(){const p=this._map.project(...this.latlng);this._map.popup.style.left=Math.max(8,Math.min((this._map.svg.clientWidth-220),p.x+12))+'px';this._map.popup.style.top=Math.max(8,p.y-55)+'px'}
    redraw(){if(!this._map)return;const p=this._map.project(...this.latlng);this.g.setAttribute('transform',`translate(${p.x} ${p.y})`);if(!this.g.parentNode)this._map.layerRoot.appendChild(this.g);if(this._map.popup.style.display!=='none')this.positionPopup()}
    remove(){this.g.remove();if(this._map?.popup)this._map.popup.style.display='none'}
  }
  class Polyline {
    constructor(points,opt={}){this.points=points;this.opt={...opt};this.path=svgEl('path',{class:'map-feature',fill:'none'});}
    addTo(target){if(target instanceof LayerGroup)return target.addLayer(this);target.addLayer(this);return this}
    redraw(){if(!this._map)return;const d=this.points.map((p,i)=>{const q=this._map.project(p[0],p[1]);return (i?'L':'M')+` ${q.x} ${q.y}`}).join(' ');this.path.setAttribute('d',d);this.path.setAttribute('stroke',this.opt.color||'#54d6a0');this.path.setAttribute('stroke-width',this.opt.weight||3);this.path.setAttribute('stroke-linecap','round');this.path.setAttribute('stroke-linejoin','round');if(this.opt.dashArray)this.path.setAttribute('stroke-dasharray',this.opt.dashArray);if(!this.path.parentNode)this._map.layerRoot.appendChild(this.path)}
    remove(){this.path.remove()}
  }
  function strip(s){return String(s).replace(/<[^>]*>/g,' ').replace(/&[^;]+;/g,' ').replace(/\s+/g,' ').trim()}
  window.L={
    map:id=>new MapMock(id),
    tileLayer:()=>({addTo:()=>{}}),
    control:{zoom:()=>({addTo:()=>{}})},
    layerGroup:()=>new LayerGroup(),
    rectangle:(b,o)=>new Rectangle(b,o),
    marker:(ll,o)=>new Marker(ll,o),
    polyline:(pts,o)=>new Polyline(pts,o),
    divIcon:o=>o,
    DomEvent:{stopPropagation:e=>e?.stopPropagation?.()}
  };
})();
