#pragma once

#include "AppContext.h"
#include "HtmlHelpers.h"
#include "ChainOptimizer.h"
#include <mutex>
#include <sstream>

inline void registerOptimizerRoutes(httplib::Server& svr, AppContext& ctx)
{
    auto& db      = ctx.defaultDb;
    auto& dbMutex = ctx.dbMutex;

    // ========== GET /optimizer — BPTT chain optimizer form ==========
    svr.Get("/optimizer", [&](const httplib::Request& req, httplib::Response& res) {
        std::ostringstream h;
        h << std::fixed << std::setprecision(17);
        h << html::msgBanner(req) << html::errBanner(req);
        h << "<h1>&#8711; Chain Optimizer &mdash; BPTT</h1>"
             "<p style='color:#64748b;font-size:0.85em;'>"
             "Backpropagation Through Time over the chain recurrence "
             "T<sub>c+1</sub> = T<sub>c</sub> + &Pi;<sub>c</sub>(1 &minus; s<sub>save</sub>). "
             "Computes &part;J/&part;&theta; for all optimisable parameters and runs "
             "Adam gradient ascent/descent toward the selected objective (&sect;15.9, &sect;16)."
             "</p>";

        double wal = db.loadWalletBalance();

        h << "<form class='card' method='POST' action='/optimizer/run'>"
             "<h3>Market &amp; Capital</h3>"
             "<label>Symbol</label>"
             "<input type='text' name='symbol' value='BTC'><br>"
             "<label>Reference Price</label>"
             "<input type='number' name='price' step='any' value='100000' required><br>"
             "<label>Starting Capital</label>"
             "<input type='number' name='capital' step='any' value='" << wal << "' required><br>"
             "<label>Chain Cycles</label>"
             "<input type='number' name='cycles' value='5'><br>"
             "<label>Entry Levels</label>"
             "<input type='number' name='levels' value='4'><br>"
             "<h3>Price Series (optional &mdash; enables simulator mode)</h3>"
             "<p style='color:#64748b;font-size:0.78em;margin-bottom:6px;'>"
             "One per line: <code>timestamp,price</code>. "
             "When provided, the optimizer runs the full simulator against this data "
             "instead of the analytical model. Leave empty for analytical mode.</p>"
             "<textarea name='priceSeries' rows='8' cols='50' "
             "placeholder='1700000000,50000&#10;1700003600,49500&#10;1700007200,51000&#10;...' "
             "style='width:100%;font-family:monospace;font-size:0.85em;background:#0b1426;color:#cbd5e1;"
             "border:1px solid #1a2744;border-radius:4px;padding:8px;'></textarea><br>"
             "<h3>Optimisable Parameters &theta;</h3>"
             "<p style='color:#64748b;font-size:0.78em;margin-bottom:6px;'>"
             "Each parameter has <strong style='color:#ef4444;'>non-negotiable bounds</strong> "
             "(min/max the optimizer can never cross) and can be "
             "<strong>frozen</strong> to exclude from learning entirely.</p>"
             "<table style='font-size:0.85em;width:100%;'>"
             "<thead><tr>"
             "<th>Parameter</th>"
             "<th>Initial &theta;</th>"
             "<th style='color:#ef4444;'>Min (floor)</th>"
             "<th style='color:#ef4444;'>Max (ceiling)</th>"
             "<th>Freeze</th>"
             "</tr></thead><tbody>"
             "<tr><td>Surplus Rate (s)</td>"
             "<td><input type='number' name='surplus' step='any' value='0.02' style='width:100%;'></td>"
             "<td><input type='number' name='surplus_min' step='any' value='0' style='width:100%;'></td>"
             "<td><input type='number' name='surplus_max' step='any' value='1' style='width:100%;'></td>"
             "<td style='text-align:center;'><input type='checkbox' name='surplus_frozen' value='1'></td></tr>"
             "<tr><td>Risk Coefficient (r)</td>"
             "<td><input type='number' name='risk' step='any' value='0.5' style='width:100%;'></td>"
             "<td><input type='number' name='risk_min' step='any' value='0' style='width:100%;'></td>"
             "<td><input type='number' name='risk_max' step='any' value='1' style='width:100%;'></td>"
             "<td style='text-align:center;'><input type='checkbox' name='risk_frozen' value='1'></td></tr>"
             "<tr><td>Steepness (&alpha;)</td>"
             "<td><input type='number' name='steepness' step='any' value='6' style='width:100%;'></td>"
             "<td><input type='number' name='steepness_min' step='any' value='0.1' style='width:100%;'></td>"
             "<td><input type='number' name='steepness_max' step='any' value='50' style='width:100%;'></td>"
             "<td style='text-align:center;'><input type='checkbox' name='steepness_frozen' value='1'></td></tr>"
             "<tr><td>Fee Hedging (f<sub>h</sub>)</td>"
             "<td><input type='number' name='feeHedging' step='any' value='1' style='width:100%;'></td>"
             "<td><input type='number' name='feeHedging_min' step='any' value='0.1' style='width:100%;'></td>"
             "<td><input type='number' name='feeHedging_max' step='any' value='10' style='width:100%;'></td>"
             "<td style='text-align:center;'><input type='checkbox' name='feeHedging_frozen' value='1'></td></tr>"
             "<tr><td>Max Risk (R<sub>max</sub>)</td>"
             "<td><input type='number' name='maxRisk' step='any' value='0' style='width:100%;'></td>"
             "<td><input type='number' name='maxRisk_min' step='any' value='0' style='width:100%;'></td>"
             "<td><input type='number' name='maxRisk_max' step='any' value='1' style='width:100%;'></td>"
             "<td style='text-align:center;'><input type='checkbox' name='maxRisk_frozen' value='1'></td></tr>"
             "<tr><td>Savings Rate (s<sub>save</sub>)</td>"
             "<td><input type='number' name='savingsRate' step='any' value='0.05' style='width:100%;'></td>"
             "<td><input type='number' name='savingsRate_min' step='any' value='0' style='width:100%;'></td>"
             "<td><input type='number' name='savingsRate_max' step='any' value='1' style='width:100%;'></td>"
             "<td style='text-align:center;'><input type='checkbox' name='savingsRate_frozen' value='1'></td></tr>"
             "</tbody></table>"
             "<label>Min Risk (R<sub>min</sub>)</label>"
             "<input type='number' name='minRisk' step='any' value='0'><br>"
             "<h3>Fixed Parameters</h3>"
             "<label>Fee Spread (f<sub>s</sub>)</label>"
             "<input type='number' name='feeSpread' step='any' value='0.001'><br>"
             "<label>Delta Time</label>"
             "<input type='number' name='deltaTime' step='any' value='1'><br>"
             "<label>Symbol Count</label>"
             "<input type='number' name='symbolCount' value='1'><br>"
             "<label>Coefficient K</label>"
             "<input type='number' name='coefficientK' step='any' value='0'><br>"
             "<label>Buy Fee Rate</label>"
             "<input type='number' name='buyFeeRate' step='any' value='0.001'><br>"
             "<label>Sell Fee Rate</label>"
             "<input type='number' name='sellFeeRate' step='any' value='0.001'><br>"
             "<label>Range Above</label>"
             "<input type='number' name='rangeAbove' step='any' value='0'><br>"
             "<label>Range Below</label>"
             "<input type='number' name='rangeBelow' step='any' value='0'><br>"
             "<label>Future Trade Count</label>"
             "<input type='number' name='futureTradeCount' value='0'><br>"
             "<label>SL Fraction</label>"
             "<input type='number' name='stopLossFraction' step='any' value='1'><br>"
             "<label>SL Hedge Count</label>"
             "<input type='number' name='stopLossHedgeCount' value='0'><br>"
             "<h3>Exit Parameters</h3>"
             "<label>Exit Risk</label>"
             "<input type='number' name='exitRisk' step='any' value='0.5'><br>"
             "<label>Exit Fraction</label>"
             "<input type='number' name='exitFraction' step='any' value='1'><br>"
             "<label>Exit Steepness</label>"
             "<input type='number' name='exitSteepness' step='any' value='4'><br>"
             "<label>DT Count</label>"
             "<input type='number' name='downtrendCount' value='1'><br>"
             "<h3>Optimisation</h3>"
             "<label>Objective</label><select name='objective'>"
             "<option value='1'>J1: MaxProfit</option>"
             "<option value='2'>J2: MinSpread</option>"
             "<option value='3'>J3: MaxROI</option>"
             "<option value='4'>J4: MaxChain</option>"
             "<option value='5' selected>J5: MaxWealth</option>"
             "</select><br>"
             "<label>Learning Rate</label>"
             "<input type='number' name='learningRate' step='any' value='0.001'><br>"
             "<label>Max Steps</label>"
             "<input type='number' name='maxSteps' value='50'><br>"
             "<h3>Multi-Chain Execution</h3>"
             "<p style='color:#64748b;font-size:0.78em;margin-bottom:6px;'>"
             "Run multiple sequential chains. Each chain takes the optimised &theta; and "
             "final capital from the previous chain as its starting point. "
             "1 = single chain (default).</p>"
             "<label>Meta-Chains</label>"
             "<input type='number' name='metaChains' value='1' min='1' max='100'><br>";

        // GPU acceleration toggle (only show if CUDA is compiled in)
        if (CudaAccelerator::isAvailable())
        {
            h << "<h3>GPU Acceleration</h3>"
                  "<div class='msg' style='border-color:#22c55e;'>"
                  "&#x1F7E2; CUDA GPU detected: <strong>"
               << html::esc(CudaAccelerator::deviceName())
               << "</strong></div>"
                   "<label>Use CUDA for simulator-mode gradients</label>"
                   "<select name='useCuda'>"
                   "<option value='1' selected>Enabled</option>"
                   "<option value='0'>Disabled (CPU)</option>"
                   "</select><br>"
                   "<label>GPU Throttle (% of SMs &mdash; lower = desktop stays responsive)</label>"
                   "<input type='range' name='gpuThrottle' min='10' max='100' value='"
                << CudaAccelerator::getThrottle()
                << "' oninput='this.nextElementSibling.textContent=this.value+\"%\"'>"
                   "<span style='color:#cbd5e1;margin-left:8px;'>"
                << CudaAccelerator::getThrottle()
                << "%</span><br>"
                   "<label>VRAM Budget (% of GPU memory &mdash; 50% leaves room for desktop)</label>"
                   "<input type='range' name='gpuMemBudget' min='10' max='100' value='"
                << CudaAccelerator::getMemBudget()
                << "' oninput='this.nextElementSibling.textContent=this.value+\"%\"'>"
                   "<span style='color:#cbd5e1;margin-left:8px;'>"
                << CudaAccelerator::getMemBudget()
                << "%</span><br>";
        }

        h << "<br><button>Run BPTT Optimisation</button></form>";

        res.set_content(html::wrap("Chain Optimizer", h.str()), "text/html");
    });

    // ========== POST /optimizer/run — execute BPTT with real-time streaming ==========
    svr.Post("/optimizer/run", [&](const httplib::Request& req, httplib::Response& res) {
        auto f = parseForm(req.body);

        ChainParams cp;
        cp.price           = fd(f, "price", 100000);
        cp.capital         = fd(f, "capital", 1000);
        cp.cycles          = fi(f, "cycles", 5);
        cp.levels          = fi(f, "levels", 4);
        cp.surplus         = fd(f, "surplus", 0.02);
        cp.risk            = fd(f, "risk", 0.5);
        cp.steepness       = fd(f, "steepness", 6.0);
        cp.feeHedging      = fd(f, "feeHedging", 1.0);
        cp.maxRisk         = fd(f, "maxRisk");
        cp.minRisk         = fd(f, "minRisk");
        cp.savingsRate     = fd(f, "savingsRate", 0.05);

        // Parse non-negotiable parameter bounds and freeze flags
        cp.bSurplus     = { fd(f, "surplus_min", 0.0),     fd(f, "surplus_max", 1.0),     fv(f, "surplus_frozen") == "1" };
        cp.bRisk        = { fd(f, "risk_min", 0.0),        fd(f, "risk_max", 1.0),        fv(f, "risk_frozen") == "1" };
        cp.bSteepness   = { fd(f, "steepness_min", 0.1),   fd(f, "steepness_max", 50.0),  fv(f, "steepness_frozen") == "1" };
        cp.bFeeHedging  = { fd(f, "feeHedging_min", 0.1),  fd(f, "feeHedging_max", 10.0), fv(f, "feeHedging_frozen") == "1" };
        cp.bMaxRisk     = { fd(f, "maxRisk_min", 0.0),     fd(f, "maxRisk_max", 1.0),     fv(f, "maxRisk_frozen") == "1" };
        cp.bSavingsRate = { fd(f, "savingsRate_min", 0.0),  fd(f, "savingsRate_max", 1.0), fv(f, "savingsRate_frozen") == "1" };
        cp.feeSpread       = fd(f, "feeSpread", 0.001);
        cp.deltaTime       = fd(f, "deltaTime", 1.0);
        cp.symbolCount     = fi(f, "symbolCount", 1);
        cp.coefficientK    = fd(f, "coefficientK");
        cp.buyFeeRate      = fd(f, "buyFeeRate", 0.001);
        cp.sellFeeRate     = fd(f, "sellFeeRate", 0.001);
        cp.rangeAbove      = fd(f, "rangeAbove");
        cp.rangeBelow      = fd(f, "rangeBelow");
        cp.futureTradeCount   = fi(f, "futureTradeCount");
        cp.stopLossFraction   = fd(f, "stopLossFraction", 1.0);
        cp.stopLossHedgeCount = fi(f, "stopLossHedgeCount");
        cp.exitRisk           = fd(f, "exitRisk", 0.5);
        cp.exitFraction       = fd(f, "exitFraction", 1.0);
        cp.exitSteepness      = fd(f, "exitSteepness", 4.0);
        cp.downtrendCount     = fi(f, "downtrendCount", 1);

        // Parse symbol and price series
        cp.symbol = normalizeSymbol(fv(f, "symbol"));
        if (cp.symbol.empty()) cp.symbol = "BTC";
        {
            std::string priceStr = fv(f, "priceSeries");
            if (!priceStr.empty()) {
                std::istringstream ss(priceStr);
                std::string line;
                while (std::getline(ss, line)) {
                    if (line.empty()) continue;
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    auto comma = line.find(',');
                    if (comma == std::string::npos) continue;
                    try {
                        long long ts = std::stoll(line.substr(0, comma));
                        double px    = std::stod(line.substr(comma + 1));
                        cp.prices.set(cp.symbol, ts, px);
                    } catch (...) {}
                }
            }
        }

        int objInt = fi(f, "objective", 5);
        auto obj   = static_cast<ChainObjective>(
            std::max(1, std::min(5, objInt)));

        double lr       = fd(f, "learningRate", 0.001);
        int    maxSteps = fi(f, "maxSteps", 50);
        int    metaChains = std::max(1, std::min(100, fi(f, "metaChains", 1)));
        bool   useCuda   = (fv(f, "useCuda") == "1") && CudaAccelerator::isAvailable();
        cp.useCuda = useCuda;

        // Apply GPU throttle and memory budget before the run
        if (useCuda)
        {
            CudaAccelerator::setThrottle(fi(f, "gpuThrottle", 100));
            CudaAccelerator::setMemBudget(fi(f, "gpuMemBudget", 50));
        }

        if (cp.price <= 0 || cp.capital <= 0) {
            res.set_redirect("/optimizer?err=Price+and+capital+must+be+positive", 303);
            return;
        }

        const char* objNames[] = {"", "MaxProfit", "MinSpread", "MaxROI", "MaxChain", "MaxWealth"};
        std::string objName = objNames[objInt];
        bool simMode = cp.hasPriceSeries();
        int priceCount = simMode
            ? static_cast<int>(cp.prices.data().at(cp.symbol).size()) : 0;

        // ---- Stream the response using chunked transfer ----
        res.set_chunked_content_provider("text/html",
            [cp, obj, objName, lr, maxSteps, simMode, priceCount, metaChains, useCuda]
            (size_t /*offset*/, httplib::DataSink& sink) {

            auto emit = [&](const std::string& s) {
                sink.write(s.data(), s.size());
            };

            // Phase 1: HTML page shell with live-updating JS
            {
                std::ostringstream hdr;
                hdr << html::wrapOpen("BPTT Results - " + objName);
                hdr << "<h1>&#8711; BPTT Results &mdash; " << objName << "</h1>";
                if (metaChains > 1)
                    hdr << "<div class='msg' style='border-color:#c9a44a;'>"
                           "Multi-chain execution: <strong>" << metaChains
                        << " sequential chains</strong>. Each chain inherits "
                           "&theta; and capital from the previous.</div>";
                if (simMode)
                    hdr << "<div class='msg'>Simulator mode &mdash; "
                        << priceCount << " price points for " << html::esc(cp.symbol)
                        << ". Gradients via end-to-end finite differences."
                        << (useCuda ? " <strong style='color:#22c55e;'>&#x26A1; CUDA GPU</strong>" : "")
                        << "</div>";
                else
                    hdr << "<div class='msg'>Analytical mode &mdash; all TPs assumed hit. "
                           "Gradients from &sect;15 closed-form + numerical.</div>";

                // Live stats bar
                hdr << "<div class='row' id='liveRow'>"
                       "<div class='stat'><div class='lbl'>Objective</div><div class='val'>"
                    << objName << "</div></div>"
                       "<div class='stat'><div class='lbl'>Step</div><div class='val' id='liveStep'>0</div></div>"
                       "<div class='stat'><div class='lbl'>J(&theta;)</div><div class='val' id='liveJ'>&mdash;</div></div>"
                       "<div class='stat'><div class='lbl'>&Delta;J</div><div class='val' id='liveDJ'>&mdash;</div></div>"
                       "<div class='stat'><div class='lbl'>||&nabla;J||</div><div class='val' id='liveGrad'>&mdash;</div></div>"
                       "<div class='stat'><div class='lbl'>Status</div><div class='val' id='liveStatus' "
                       "style='color:#60a5fa;'>&#9881; Running...</div></div>"
                       "</div>";

                // Convergence chart canvas
                hdr << "<h2>Convergence</h2>"
                       "<div style='position:relative;width:100%;height:300px;"
                       "background:#0b1426;border:1px solid #1a2744;border-radius:6px;margin:16px 0;'>"
                       "<canvas id='bpttCanvas' style='display:block;width:100%;height:100%;'></canvas>"
                       "<div id='bpttTip' style='position:absolute;background:#0f1b2dee;"
                       "border:1px solid #1a2744;border-radius:6px;padding:6px 10px;"
                       "font-size:0.75em;color:#cbd5e1;pointer-events:none;display:none;"
                       "white-space:pre;font-family:monospace;z-index:10;'></div>"
                       "</div>"
                       "<div style='display:flex;gap:16px;font-size:0.72em;padding:4px 12px;color:#64748b;'>"
                       "<span>&#9632; <span style='color:#c9a44a'>J(&theta;)</span></span>"
                       "<span>&#9632; <span style='color:#60a5fa'>||&nabla;J||</span></span>"
                       "</div>";

                // Learning curve table shell
                hdr << "<h2>Learning Curve <span id='lcCount'></span></h2>"
                       "<div style='overflow-x:auto;'>"
                       "<table style='font-size:0.78em;'><thead><tr>"
                       "<th>Step</th><th>J(&theta;)</th><th>&Delta;J</th><th>||&nabla;J||</th>"
                       "<th>s</th><th>r</th><th>&alpha;</th><th>f<sub>h</sub></th>"
                       "<th>R<sub>max</sub></th><th>s<sub>save</sub></th>"
                       "<th>&part;J/&part;s</th><th>&part;J/&part;r</th>"
                       "<th>&part;J/&part;&alpha;</th></tr></thead>"
                       "<tbody id='lcBody'></tbody></table></div>";

                // Placeholder for final sections
                hdr << "<div id='finalSections'></div>";

                // JavaScript: addStep function + chart drawing
                hdr << R"(<script>
var bpttHist=[], bpttGrad=[];
function addStep(d){
  bpttHist.push(d.J); bpttGrad.push(d.gn);
  document.getElementById('liveStep').textContent=d.step;
  document.getElementById('liveJ').textContent=d.J.toFixed(6);
  document.getElementById('liveDJ').textContent=(d.dJ>=0?'+':'')+d.dJ.toFixed(6);
  document.getElementById('liveDJ').className='val '+(d.dJ>=0?'buy':'sell');
  document.getElementById('liveGrad').textContent=d.gn.toExponential(3);
  document.getElementById('lcCount').textContent='('+bpttHist.length+' steps)';
  var tb=document.getElementById('lcBody');
  var tr=document.createElement('tr');
  var vals=[d.step,d.J.toFixed(8),(d.dJ>=0?'+':'')+d.dJ.toFixed(8),d.gn.toExponential(4),
    d.s.toFixed(8),d.r.toFixed(8),d.a.toFixed(6),d.fh.toFixed(6),d.rm.toFixed(6),d.sv.toFixed(6),
    d.gs.toFixed(4),d.gr.toFixed(4),d.ga.toFixed(4)];
  for(var i=0;i<vals.length;i++){var td=document.createElement('td');td.textContent=vals[i];
    if(i===2)td.className=d.dJ>=0?'buy':'sell';tr.appendChild(td);}
  tb.appendChild(tr);
  tr.scrollIntoView({block:'nearest',behavior:'smooth'});
  drawChart();
}
function drawChart(){
  var cv=document.getElementById('bpttCanvas');
  if(!cv||bpttHist.length<1)return;
  var dpr=window.devicePixelRatio||1;
  var rc=cv.getBoundingClientRect();
  cv.width=rc.width*dpr;cv.height=rc.height*dpr;
  var ctx=cv.getContext('2d');ctx.scale(dpr,dpr);
  var w=rc.width,h=rc.height;
  var pad={l:80,r:80,t:20,b:40};
  var pw=w-pad.l-pad.r,ph=h-pad.t-pad.b;
  var obj=bpttHist,grd=bpttGrad;
  var jmn=obj[0],jmx=obj[0];
  for(var i=1;i<obj.length;i++){if(obj[i]<jmn)jmn=obj[i];if(obj[i]>jmx)jmx=obj[i];}
  if(jmn===jmx){jmn-=1;jmx+=1;}
  var jr=jmx-jmn;jmn-=jr*0.05;jmx+=jr*0.05;
  var gmn=0,gmx=0;
  for(var i=0;i<grd.length;i++){if(grd[i]>gmx)gmx=grd[i];}
  if(gmx===0)gmx=1;gmx*=1.1;
  ctx.fillStyle='#0b1426';ctx.fillRect(0,0,w,h);
  ctx.strokeStyle='#1a2744';ctx.lineWidth=1;
  for(var g=0;g<=4;g++){
    var gy=pad.t+ph*(1-g/4);
    ctx.beginPath();ctx.moveTo(pad.l,gy);ctx.lineTo(pad.l+pw,gy);ctx.stroke();
    ctx.fillStyle='#c9a44a';ctx.font='11px monospace';ctx.textAlign='right';
    ctx.fillText((jmn+(jmx-jmn)*g/4).toFixed(4),pad.l-6,gy+4);
    ctx.fillStyle='#60a5fa';ctx.textAlign='left';
    ctx.fillText((gmn+(gmx-gmn)*g/4).toExponential(2),pad.l+pw+6,gy+4);
  }
  var n=obj.length;var xStep=n>1?pw/(n-1):0;
  ctx.strokeStyle='#c9a44a';ctx.lineWidth=2;ctx.beginPath();
  for(var i=0;i<n;i++){var x=pad.l+i*xStep;var y=pad.t+ph*(1-(obj[i]-jmn)/(jmx-jmn));
    if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}
  ctx.stroke();
  if(grd.length>1){ctx.strokeStyle='#60a5fa';ctx.lineWidth=1.5;ctx.setLineDash([4,3]);ctx.beginPath();
    for(var i=0;i<grd.length;i++){var x=pad.l+i*(grd.length>1?pw/(grd.length-1):0);
      var y=pad.t+ph*(1-(grd[i]-gmn)/(gmx-gmn));
      if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}
    ctx.stroke();ctx.setLineDash([]);}
  for(var i=0;i<n;i++){var x=pad.l+i*xStep;var y=pad.t+ph*(1-(obj[i]-jmn)/(jmx-jmn));
    ctx.fillStyle='#c9a44a';ctx.beginPath();ctx.arc(x,y,3,0,Math.PI*2);ctx.fill();}
  ctx.fillStyle='#64748b';ctx.font='11px monospace';ctx.textAlign='center';
  ctx.fillText('Optimisation Step',pad.l+pw/2,h-4);
}
window.addEventListener('resize',drawChart);
</script>)";
                emit(hdr.str());
            }

            // ---- Multi-chain execution loop ----
            ChainParams curChainParams = cp;
            double initialCapital = cp.capital;

            struct MetaChainRecord {
                int    chain        = 0;
                int    steps        = 0;
                double startCapital = 0;
                double endCapital   = 0;
                double startJ       = 0;
                double endJ         = 0;
                double totalProfit  = 0;
                double totalSavings = 0;
            };
            std::vector<MetaChainRecord> metaLog;

            for (int mc = 0; mc < metaChains; ++mc)
            {
                // ---- Chain header ----
                if (metaChains > 1)
                {
                    std::ostringstream ch;
                    ch << std::fixed << std::setprecision(6);
                    ch << "<script>document.getElementById('liveStatus').innerHTML="
                          "'&#9881; Chain " << (mc + 1) << "/" << metaChains
                       << "';document.getElementById('liveStatus').style.color='#60a5fa';</script>";
                    if (mc > 0)
                        ch << "<hr style='border-color:#1a2744;margin:24px 0;'>";
                    ch << "<h2 id='mcH" << mc << "'>&#9881; Meta-Chain " << (mc + 1)
                       << " / " << metaChains
                       << " &mdash; Capital: " << curChainParams.capital << "</h2>";
                    emit(ch.str());
                }

                // Phase 2: Run optimizer, streaming each step
                int stepOffset = 0;
                if (!metaLog.empty()) {
                    for (const auto& m : metaLog) stepOffset += m.steps;
                }

                auto result = ChainOptimizer::optimize(curChainParams, obj, maxSteps, lr,
                    [&](const StepRecord& sr) {
                        std::ostringstream js;
                        js << std::fixed << std::setprecision(17);
                        js << "<script>addStep({step:" << (stepOffset + sr.step)
                           << ",J:" << sr.objective
                           << ",dJ:" << sr.deltaJ
                           << ",gn:" << sr.gradNorm
                           << ",s:" << sr.surplus
                           << ",r:" << sr.risk
                           << ",a:" << sr.steepness
                           << ",fh:" << sr.feeHedging
                           << ",rm:" << sr.maxRisk
                           << ",sv:" << sr.savingsRate
                           << ",gs:" << sr.g_surplus
                           << ",gr:" << sr.g_risk
                           << ",ga:" << sr.g_steepness
                           << "});</script>\n";
                        emit(js.str());
                    });

                // Record meta-chain stats
                {
                    MetaChainRecord mcr;
                    mcr.chain        = mc;
                    mcr.steps        = result.steps;
                    mcr.startCapital = curChainParams.capital;
                    mcr.startJ       = result.objectiveHistory.empty() ? 0 : result.objectiveHistory.front();
                    mcr.endJ         = result.objectiveHistory.empty() ? 0 : result.objectiveHistory.back();
                    if (!result.forwardTrace.empty()) {
                        mcr.endCapital = result.forwardTrace.back().nextCapital;
                        for (const auto& cr : result.forwardTrace) {
                            mcr.totalProfit  += cr.totalProfit;
                            mcr.totalSavings += cr.savings;
                        }
                    } else {
                        mcr.endCapital = curChainParams.capital;
                    }
                    metaLog.push_back(mcr);
                }

                // Phase 3: Chain results
                {
                    std::ostringstream h;
                    h << std::fixed << std::setprecision(17);

                    if (metaChains == 1)
                    {
                        // Mark status as done
                        h << "<script>document.getElementById('liveStatus').innerHTML="
                             "'&#10003; Done';document.getElementById('liveStatus').style.color='#22c55e';</script>";
                    }

                    // Summary stats
                    h << "<script>document.getElementById('finalSections').innerHTML"
                       << (mc == 0 ? "=" : "+=") << "`";

                    if (metaChains > 1)
                        h << "<h3 style='color:#c9a44a;'>Chain " << (mc + 1)
                           << " / " << metaChains << " Results</h3>";

                    h << "<h2>Parameter Optimisation</h2>"
                         "<table><tr><th>Parameter</th><th>\\u03b8 initial</th><th>\\u03b8 optimised</th>"
                         "<th>\\u0394\\u03b8</th><th>Bounds</th><th>Status</th>"
                         "<th>\\u2202J/\\u2202\\u03b8 initial</th>"
                         "<th>\\u2202J/\\u2202\\u03b8 final</th></tr>";

                    const auto& ip = result.initialParams;
                    const auto& op = result.optimizedParams;
                    const auto& ig = result.initialGradients;
                    const auto& fg = result.finalGradients;

                    auto paramRow = [&](const char* name, double init, double opt,
                                        double gi, double gf, const ParamBound& b) {
                        h << "<tr><td>" << name << "</td>"
                          << "<td>" << init << "</td>"
                          << "<td>" << opt << "</td>"
                          << "<td class='" << (opt - init >= 0 ? "buy" : "sell") << "'>"
                          << (opt - init) << "</td>"
                          << "<td style='color:#64748b;font-size:0.82em;'>[" << b.lower << ", " << b.upper << "]</td>"
                          << "<td>" << (b.frozen ? "\\u2744 Frozen" : "\\u2714 Learnable") << "</td>"
                          << "<td>" << gi << "</td>"
                          << "<td>" << gf << "</td></tr>";
                    };

                    paramRow("s (surplus)",  ip.surplus,    op.surplus,    ig.dJ_dSurplus,    fg.dJ_dSurplus,    ip.bSurplus);
                    paramRow("r (risk)",     ip.risk,       op.risk,       ig.dJ_dRisk,       fg.dJ_dRisk,       ip.bRisk);
                    paramRow("\\u03b1 (steep)", ip.steepness, op.steepness, ig.dJ_dSteepness, fg.dJ_dSteepness, ip.bSteepness);
                    paramRow("f_h (feeHedge)", ip.feeHedging, op.feeHedging, ig.dJ_dFeeHedging, fg.dJ_dFeeHedging, ip.bFeeHedging);
                    paramRow("R_max",        ip.maxRisk,    op.maxRisk,    ig.dJ_dMaxRisk,    fg.dJ_dMaxRisk,    ip.bMaxRisk);
                    paramRow("s_save",       ip.savingsRate, op.savingsRate, ig.dJ_dSavingsRate, fg.dJ_dSavingsRate, ip.bSavingsRate);
                    h << "</table>";

                    // Forward trace
                    if (!result.forwardTrace.empty())
                    {
                        h << "<h2>Optimised Forward Trace (" << result.forwardTrace.size() << " cycles)</h2>"
                             "<table><tr><th>Cycle</th><th>Capital T_c</th>"
                             "<th>Profit \\u03a0_c</th><th>Savings</th>"
                             "<th>T_c+1</th><th>Levels</th>"
                             "<th>\\u2202\\u03a0/\\u2202T</th><th>\\u2202\\u03a0/\\u2202s</th></tr>";
                        for (int c = 0; c < static_cast<int>(result.forwardTrace.size()); ++c)
                        {
                            const auto& cr = result.forwardTrace[c];
                            h << "<tr><td>" << c << "</td>"
                              << "<td>" << cr.capital << "</td>"
                              << "<td class='" << (cr.totalProfit >= 0 ? "buy" : "sell") << "'>"
                              << cr.totalProfit << "</td>"
                              << "<td>" << cr.savings << "</td>"
                              << "<td>" << cr.nextCapital << "</td>"
                              << "<td>" << cr.levels.size() << "</td>"
                              << "<td>" << cr.dProfit_dCapital << "</td>"
                              << "<td>" << cr.dProfit_dSurplus << "</td></tr>";
                        }
                        h << "</table>";
                    }

                    // Gradient summary
                    h << "<h2>Gradient Summary</h2>"
                         "<table><tr><th>Output</th><th>w.r.t.</th><th>Sign</th>"
                         "<th>Value</th><th>Interpretation</th></tr>";
                    auto gradRow = [&](const char* out, const char* wrt, double val, const char* interp) {
                        const char* sign = (val > 0) ? "+" : (val < 0) ? "-" : "0";
                        h << "<tr><td>" << out << "</td><td>" << wrt << "</td>"
                          << "<td>" << sign << "</td><td>" << val << "</td>"
                          << "<td style='color:#64748b;font-size:0.82em;'>" << interp << "</td></tr>";
                    };
                    gradRow("J", "s",      fg.dJ_dSurplus,    "Surplus raises TP floor");
                    gradRow("J", "r",      fg.dJ_dRisk,       "Risk reallocates capital");
                    gradRow("J", "\\u03b1", fg.dJ_dSteepness, "Steepness shifts entry distribution");
                    gradRow("J", "f_h",    fg.dJ_dFeeHedging, "Fee hedging coefficient");
                    gradRow("J", "R_max",  fg.dJ_dMaxRisk,    "Ceiling scales with reference");
                    gradRow("J", "s_save", fg.dJ_dSavingsRate, "Savings drain capital, lock profit");
                    h << "</table>";

                    // Level detail (last cycle)
                    if (!result.forwardTrace.empty())
                    {
                        const auto& last = result.forwardTrace.back();
                        if (!last.levels.empty())
                        {
                            h << "<h2>Level Detail (Cycle " << (result.forwardTrace.size() - 1) << ")</h2>"
                                 "<table><tr><th>Level</th><th>Entry</th><th>TP</th><th>Qty</th>"
                                 "<th>Funding</th><th>OH</th><th>EO</th>"
                                 "<th>Gross</th><th>Net</th><th>Buy Time</th><th>Sell Time</th><th>Status</th></tr>";
                            for (size_t i = 0; i < last.levels.size(); ++i)
                            {
                                const auto& l = last.levels[i];
                                bool sold = (l.sellFee > 0 || l.grossProfit != 0 || l.netProfit != 0);

                                // Format timestamps as ISO date-time strings
                                // Uses gmtime for post-epoch, Howard Hinnant's civil_from_days for pre-epoch
                                auto fmtTime = [](long long ts) -> std::string {
                                    if (ts == 0) return "&mdash;";
                                    std::time_t t = static_cast<std::time_t>(ts);
                                    std::tm tm{};
                                    bool ok = false;
#ifdef _WIN32
                                    ok = (gmtime_s(&tm, &t) == 0);
#else
                                    ok = (gmtime_r(&t, &tm) != nullptr);
#endif
                                    if (ok) {
                                        char buf[32];
                                        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
                                        return buf;
                                    }
                                    // Fallback for pre-epoch dates (gmtime_s fails for negative time_t on Windows)
                                    long long sod = ((ts % 86400) + 86400) % 86400;
                                    long long z = (ts - sod) / 86400 + 719468;
                                    long long era = (z >= 0 ? z : z - 146096) / 146097;
                                    long long doe = z - era * 146097;
                                    long long yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
                                    long long y = yoe + era * 400;
                                    long long doy = doe - (365*yoe + yoe/4 - yoe/100);
                                    long long mp = (5*doy + 2) / 153;
                                    int d = static_cast<int>(doy - (153*mp + 2) / 5 + 1);
                                    int m = static_cast<int>(mp + (mp < 10 ? 3 : -9));
                                    y += (m <= 2);
                                    char buf[32];
                                    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d",
                                             static_cast<int>(y), m, d,
                                             static_cast<int>(sod / 3600),
                                             static_cast<int>((sod % 3600) / 60));
                                    return buf;
                                };

                                h << "<tr><td>" << i << "</td>"
                                  << "<td>" << l.entryPrice << "</td>"
                                  << "<td" << (sold ? "" : " style='color:#64748b;font-style:italic;'") << ">"
                                  << l.tpPrice << "</td>"
                                  << "<td>" << l.quantity << "</td>"
                                  << "<td>" << l.funding << "</td>"
                                  << "<td>" << l.overhead << "</td>"
                                  << "<td>" << l.effectiveOH << "</td>"
                                  << "<td>" << l.grossProfit << "</td>"
                                  << "<td class='" << (l.netProfit >= 0 ? "buy" : "sell") << "'>"
                                  << l.netProfit << "</td>"
                                  << "<td style='font-size:0.82em;' title='ts=" << l.entryTime << "'>" << fmtTime(l.entryTime) << "</td>"
                                  << "<td style='font-size:0.82em;' title='ts=" << l.sellTime << "'>" << fmtTime(l.sellTime) << "</td>"
                                  << "<td>" << (sold ? "\\u2714 Sold" : "\\u23F3 Open") << "</td></tr>";
                            }
                            h << "</table>";
                        }
                    }

                    h << "`;</script>";

                    // Trade visualization chart (sim mode only)
                    if (simMode && !result.simResult.trades.empty()
                        && curChainParams.prices.hasSymbol(curChainParams.symbol))
                    {
                    // Serialize price series
                    h << "<h2>Trade Visualisation"
                       << (metaChains > 1 ? " (Chain " + std::to_string(mc + 1) + ")" : "")
                       << "</h2>"
                          "<div style='position:relative;width:100%;height:400px;"
                          "background:#0b1426;border:1px solid #1a2744;border-radius:6px;margin:16px 0;'>"
                          "<canvas id='tradeCanvas" << mc << "' style='display:block;width:100%;height:100%;'></canvas>"
                          "<div id='tradeTip" << mc << "' style='position:absolute;background:#0f1b2dee;"
                          "border:1px solid #1a2744;border-radius:6px;padding:6px 10px;"
                          "font-size:0.75em;color:#cbd5e1;pointer-events:none;display:none;"
                          "white-space:pre;font-family:monospace;z-index:10;'></div>"
                          "</div>"
                          "<div style='display:flex;gap:16px;font-size:0.72em;padding:4px 12px;color:#64748b;'>"
                          "<span>&#9632; <span style='color:#64748b'>Price</span></span>"
                          "<span>&#9650; <span style='color:#22c55e'>Buy</span></span>"
                          "<span>&#9660; <span style='color:#c9a44a'>Sell</span></span>"
                          "</div>"
                          "<div style='font-size:0.78em;color:#64748b;margin:4px 12px;'>"
                          "Trades: " << result.simResult.tradesOpened
                       << " opened, " << result.simResult.tradesClosed << " closed"
                       << " | Wins: " << result.simResult.wins
                       << " | Realised: <span class='"
                       << (result.simResult.totalRealized >= 0 ? "buy" : "sell") << "'>"
                       << result.simResult.totalRealized << "</span>"
                       << " | Fees: " << result.simResult.totalFees
                       << "</div>";

                    // Serialize data arrays as JS
                    h << "<script>(function(){"
                          "var px=[";
                    {
                        const auto& pts = curChainParams.prices.data().at(curChainParams.symbol);
                        for (size_t i = 0; i < pts.size(); ++i) {
                            if (i > 0) h << ',';
                            h << '[' << pts[i].timestamp << ',' << pts[i].price << ']';
                        }
                    }
                    h << "];var buys=[";
                    for (size_t i = 0; i < result.simResult.trades.size(); ++i) {
                        const auto& t = result.simResult.trades[i];
                        if (i > 0) h << ',';
                        h << "[" << t.entryTime << "," << t.entryPrice
                          << "," << t.quantity << "," << t.cycle
                          << "," << t.id << "]";
                    }
                    h << "];var sells=[";
                    for (size_t i = 0; i < result.simResult.sells.size(); ++i) {
                        const auto& s = result.simResult.sells[i];
                        if (i > 0) h << ',';
                        h << "[" << s.sellTime << "," << s.sellPrice
                          << "," << s.netProfit << "," << s.cycle
                          << "," << s.buyId << "]";
                    }
                    h << "];";

                    // Chart drawing code (canvas ID indexed by chain)
                    h << "var cv=document.getElementById('tradeCanvas" << mc << "');"
                          "var tip=document.getElementById('tradeTip" << mc << "');";
                    h << R"JS(
if(!cv||px.length<2){return;}
function draw(){
  var dpr=window.devicePixelRatio||1;
  var rc=cv.getBoundingClientRect();
  cv.width=rc.width*dpr;cv.height=rc.height*dpr;
  var ctx=cv.getContext('2d');ctx.scale(dpr,dpr);
  var w=rc.width,h=rc.height;
  var pad={l:70,r:20,t:20,b:40};
  var pw=w-pad.l-pad.r,ph=h-pad.t-pad.b;
  var tMin=px[0][0],tMax=px[px.length-1][0];
  var pMin=px[0][1],pMax=px[0][1];
  for(var i=1;i<px.length;i++){if(px[i][1]<pMin)pMin=px[i][1];if(px[i][1]>pMax)pMax=px[i][1];}
  for(var i=0;i<buys.length;i++){if(buys[i][1]<pMin)pMin=buys[i][1];if(buys[i][1]>pMax)pMax=buys[i][1];}
  for(var i=0;i<sells.length;i++){if(sells[i][1]<pMin)pMin=sells[i][1];if(sells[i][1]>pMax)pMax=sells[i][1];}
  var pr=pMax-pMin;pMin-=pr*0.05;pMax+=pr*0.05;
  var tRange=tMax-tMin||1;
  function tx(t){return pad.l+pw*(t-tMin)/tRange;}
  function ty(p){return pad.t+ph*(1-(p-pMin)/(pMax-pMin));}
  ctx.fillStyle='#0b1426';ctx.fillRect(0,0,w,h);
  ctx.strokeStyle='#1a2744';ctx.lineWidth=1;
  for(var g=0;g<=4;g++){
    var gy=pad.t+ph*(1-g/4);
    ctx.beginPath();ctx.moveTo(pad.l,gy);ctx.lineTo(pad.l+pw,gy);ctx.stroke();
    ctx.fillStyle='#64748b';ctx.font='10px monospace';ctx.textAlign='right';
    ctx.fillText((pMin+(pMax-pMin)*g/4).toFixed(2),pad.l-6,gy+4);
  }
  ctx.strokeStyle='#334155';ctx.lineWidth=1.5;ctx.beginPath();
  for(var i=0;i<px.length;i++){
    var x=tx(px[i][0]),y=ty(px[i][1]);
    if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
  }
  ctx.stroke();
  var ccol=['#3b82f6','#8b5cf6','#06b6d4','#f59e0b','#ec4899','#10b981'];
  for(var i=0;i<sells.length;i++){
    var s=sells[i];
    for(var j=0;j<buys.length;j++){
      if(buys[j][4]===s[4]){
        var bx=tx(buys[j][0]),by=ty(buys[j][1]);
        var sx=tx(s[0]),sy=ty(s[1]);
        ctx.strokeStyle=s[2]>=0?'#22c55e44':'#ef444444';
        ctx.lineWidth=1;ctx.setLineDash([3,3]);
        ctx.beginPath();ctx.moveTo(bx,by);ctx.lineTo(sx,sy);ctx.stroke();
        ctx.setLineDash([]);
        break;
      }
    }
  }
  for(var i=0;i<buys.length;i++){
    var x=tx(buys[i][0]),y=ty(buys[i][1]);
    var c=buys[i][3]%ccol.length;
    ctx.fillStyle='#22c55e';ctx.strokeStyle=ccol[c];ctx.lineWidth=1.5;
    ctx.beginPath();ctx.moveTo(x,y-7);ctx.lineTo(x-5,y+4);ctx.lineTo(x+5,y+4);ctx.closePath();
    ctx.fill();ctx.stroke();
  }
  for(var i=0;i<sells.length;i++){
    var x=tx(sells[i][0]),y=ty(sells[i][1]);
    var c=sells[i][3]%ccol.length;
    var col=sells[i][2]>=0?'#c9a44a':'#ef4444';
    ctx.fillStyle=col;ctx.strokeStyle=ccol[c];ctx.lineWidth=1.5;
    ctx.beginPath();ctx.moveTo(x,y+7);ctx.lineTo(x-5,y-4);ctx.lineTo(x+5,y-4);ctx.closePath();
    ctx.fill();ctx.stroke();
  }
  ctx.fillStyle='#64748b';ctx.font='10px monospace';ctx.textAlign='center';
  for(var g=0;g<=4;g++){
    var t=tMin+tRange*g/4;
    var d=new Date(t*1000);
    var lbl=d.getFullYear()+'-'+(''+(d.getMonth()+1)).padStart(2,'0')+'-'+(''+(d.getDate())).padStart(2,'0');
    ctx.fillText(lbl,pad.l+pw*g/4,h-pad.b+16);
  }
  ctx.fillText('Time',pad.l+pw/2,h-4);
  cv._td={pad:pad,pw:pw,ph:ph,pMin:pMin,pMax:pMax,tMin:tMin,tRange:tRange};
}
draw();window.addEventListener('resize',draw);
cv.addEventListener('mousemove',function(e){
  var m=cv._td;if(!m)return;
  var rect=cv.getBoundingClientRect();
  var mx=e.clientX-rect.left,my=e.clientY-rect.top;
  var best=null,bestD=20;
  for(var i=0;i<buys.length;i++){
    var bx=m.pad.l+m.pw*(buys[i][0]-m.tMin)/m.tRange;
    var by=m.pad.t+m.ph*(1-(buys[i][1]-m.pMin)/(m.pMax-m.pMin));
    var d=Math.sqrt((mx-bx)*(mx-bx)+(my-by)*(my-by));
    if(d<bestD){bestD=d;best={type:'BUY',price:buys[i][1],qty:buys[i][2],cycle:buys[i][3]};}
  }
  for(var i=0;i<sells.length;i++){
    var sx=m.pad.l+m.pw*(sells[i][0]-m.tMin)/m.tRange;
    var sy=m.pad.t+m.ph*(1-(sells[i][1]-m.pMin)/(m.pMax-m.pMin));
    var d=Math.sqrt((mx-sx)*(mx-sx)+(my-sy)*(my-sy));
    if(d<bestD){bestD=d;best={type:'SELL',price:sells[i][1],net:sells[i][2],cycle:sells[i][3]};}
  }
  if(best){
    var s=best.type+' C'+best.cycle+'\nPrice: '+best.price.toFixed(5);
    if(best.qty)s+='\nQty: '+best.qty.toFixed(6);
    if(best.net!==undefined)s+='\nNet: '+(best.net>=0?'+':'')+best.net.toFixed(6);
    tip.style.display='block';tip.textContent=s;
    tip.style.left=(mx+12)+'px';tip.style.top=(my-30)+'px';
  }else{tip.style.display='none';}
});
cv.addEventListener('mouseleave',function(){tip.style.display='none';});
})();</script>
)JS";
                    }

                    emit(h.str());
                }

                // ---- Chain transition: carry forward optimized params + final capital ----
                if (mc + 1 < metaChains)
                {
                    const auto& op = result.optimizedParams;
                    curChainParams.surplus      = op.surplus;
                    curChainParams.risk         = op.risk;
                    curChainParams.steepness    = op.steepness;
                    curChainParams.feeHedging   = op.feeHedging;
                    curChainParams.maxRisk      = op.maxRisk;
                    curChainParams.savingsRate  = op.savingsRate;

                    if (!result.forwardTrace.empty())
                        curChainParams.capital = result.forwardTrace.back().nextCapital;
                }

            } // end multi-chain loop

            // ---- Multi-chain summary (when > 1 chain) ----
            {
                std::ostringstream h;
                h << std::fixed << std::setprecision(8);

                if (metaChains > 1)
                {
                    double totalProfit  = 0;
                    double totalSavings = 0;
                    int    totalSteps   = 0;
                    for (const auto& m : metaLog) {
                        totalProfit  += m.totalProfit;
                        totalSavings += m.totalSavings;
                        totalSteps   += m.steps;
                    }
                    double finalCap = metaLog.back().endCapital;
                    double growth   = initialCapital > 0
                        ? (finalCap - initialCapital) / initialCapital * 100.0 : 0;

                    h << "<script>document.getElementById('finalSections').innerHTML+=`"
                          "<hr style='border-color:#c9a44a;margin:32px 0;'>"
                          "<h2 style='color:#c9a44a;'>&#9776; Multi-Chain Summary ("
                       << metaChains << " chains)</h2>"
                          "<table><tr><th>Chain</th><th>Start Capital</th>"
                          "<th>End Capital</th><th>Profit</th>"
                          "<th>Savings</th><th>Steps</th>"
                          "<th>J start</th><th>J end</th></tr>";
                    for (const auto& m : metaLog) {
                        h << "<tr><td>" << (m.chain + 1) << "</td>"
                          << "<td>" << m.startCapital << "</td>"
                          << "<td>" << m.endCapital << "</td>"
                          << "<td class='" << (m.totalProfit >= 0 ? "buy" : "sell") << "'>"
                          << m.totalProfit << "</td>"
                          << "<td>" << m.totalSavings << "</td>"
                          << "<td>" << m.steps << "</td>"
                          << "<td>" << m.startJ << "</td>"
                          << "<td>" << m.endJ << "</td></tr>";
                    }
                    h << "</table>"
                          "<div class='row' style='margin-top:16px;'>"
                          "<div class='stat'><div class='lbl'>Initial Capital</div>"
                          "<div class='val'>" << initialCapital << "</div></div>"
                          "<div class='stat'><div class='lbl'>Final Capital</div>"
                          "<div class='val'>" << finalCap << "</div></div>"
                          "<div class='stat'><div class='lbl'>Total Profit</div>"
                          "<div class='val " << (totalProfit >= 0 ? "buy" : "sell") << "'>"
                       << totalProfit << "</div></div>"
                          "<div class='stat'><div class='lbl'>Total Savings</div>"
                          "<div class='val'>" << totalSavings << "</div></div>"
                          "<div class='stat'><div class='lbl'>Growth</div>"
                          "<div class='val " << (growth >= 0 ? "buy" : "sell") << "'>"
                       << growth << "%</div></div>"
                          "<div class='stat'><div class='lbl'>Total Steps</div>"
                          "<div class='val'>" << totalSteps << "</div></div>"
                          "</div>"
                          "`;</script>";
                }

                // Mark done
                h << "<script>document.getElementById('liveStatus').innerHTML="
                     "'&#10003; Done';document.getElementById('liveStatus').style.color='#22c55e';</script>";

                h << "<br><a class='btn' href='/optimizer'>Back to Optimizer</a>"
                     "</div></body></html>";
                emit(h.str());
            }

            sink.done();
            return true;
        });
    });
}
