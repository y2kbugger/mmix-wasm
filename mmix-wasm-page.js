initialMixalProgram = `argv   IS    $1
       #BYTE "UUUaaa",99,99,99,99,99,99
       LOC   #100
Main   LDOU  $255,argv,0
       TRAP  0,Fputs,StdOut
       GETA  $255,String
       TRAP  0,Fputs,StdOut
       TRAP  0,Halt,0
String BYTE  ", world",#a,0
`;

function makeWriter(elementid) {
    var element = document.getElementById(elementid);
    if (element) element.value = ''; // clear browser cache
    return function(text) {
      if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
      if (text == "Calling stub instead of signal()") return
      if (element) {
        element.value += text + "\n";
        element.scrollTop = element.scrollHeight; // focus on bottom
      }
      // if we need to put these at console for some reason
    };
}

var inputElement = document.getElementById('input');
var mmix;
var mmixal;

var rerunOldVal = "";
function rerun(){
    if (inputElement.value == rerunOldVal) return;

    document.getElementById('output').value = "";
    document.getElementById('error').value = "";
    FS.writeFile("/y2k.mms", inputElement.value);
    mmixal();
    if (document.getElementById('error').value == "")
    {
        mmix_sim(1, "doesn't make diff.mmo");
        // mmix_sim_js()
    }
    rerunOldVal = inputElement.value;
}

function mmix_sim_js(){
    // mmix_lib_initialize();
    // mmix_initialize();
    // boot:
    // mmix_boot();
    // mmix_load_file("y2k.mmo");
    // breakpoint= 0;
    // if(interacting)
    // {
    //     mmix_interact();
    // }
    // while(true){
    //     if(interrupt&&!breakpoint)
    //     {
    //         breakpoint|= trace_bit,interacting= true,interrupt= false;
    //     }
    //     else
    //     {
    //         breakpoint= 0;
    //         if(interacting)
    //         {
    //             mmix_interact();
    //         }
    //     }
    //     if(halted)
    //     {
    //         break;
    //     }
    //     do
    //     {
    //         if(!resuming)
    //             mmix_fetch_instruction();
    //         mmix_perform_instruction();
    //         mmix_trace();
    //         mmix_dynamic_trap();
    //         if(resuming&&op!=RESUME)
    //             resuming= false;
    //     }while(resuming||(!interrupt&&!breakpoint));
    //     if(interact_after_break)
    //         interacting= true,interact_after_break= false;
    //         if(g[rQ].l&g[rK].l&RE_BIT)
    //         {
    //             breakpoint|= trace_bit;
    //             goto boot;
    //         }
    // }
    // end_simulation:
    // if(profiling)
    //     mmix_profile();
    // if(interacting||profiling||showing_stats)
    //     show_stats(true);
    // mmix_finalize();

    mmix_lib_initialize();
    mmix_initialize();
    mmix_boot();
    mmix_load_file("y2k.mmo");
    while(true)
    {
        console.log('interating');
        if (halted())
        {
            console.log('halting');
            break;
        }
        mmix_fetch_instruction();
        mmix_perform_instruction();
        mmix_trace();
        mmix_dynamic_trap();
    }
    show_stats(true);
    mmix_finalize();
}

Module.print = makeWriter('output');
Module.printErr = makeWriter('error');
Module.onRuntimeInitialized = function (){
    // wait until runtimes are loaded to map these
    mmix_sim = Module.cwrap('mmix_sim', 'number', ['number', 'array']);
    mmixal = Module.cwrap('mmixal_wasm', 'number', []);
    halted = Module.cwrap('halted_wasm', 'number', []);
    print_mystr = Module.cwrap('print_mystr', 'number', ['string']);
    mmix_initialize_globals = Module.cwrap('mmix_initialize_globals', 'number', []);

    mmix_lib_initialize = Module.cwrap('mmix_lib_initialize', 'number', []);
    mmix_initialize = Module.cwrap('mmix_initialize', 'number', []);
    mmix_boot = Module.cwrap('mmix_boot', 'number', []);
    mmix_load_file = Module.cwrap('mmix_boot', 'number', ['string']);

    mmix_fetch_instruction = Module.cwrap('mmix_fetch_instruction', 'number', []);
    mmix_perform_instruction = Module.cwrap('mmix_perform_instruction', 'number', []);
    mmix_trace = Module.cwrap('mmix_trace', 'number', []);
    mmix_dynamic_trap = Module.cwrap('mmix_dynamic_trap', 'number', []);
    show_stats = Module.cwrap('show_stats', 'number', []);
    mmix_finalize = Module.cwrap('mmix_finalize', 'number', []);

    inputElement.value = initialMixalProgram;
}
$("#input").on('keyup paste', rerun);

// // Create a Web Worker (separate thread) that we'll pass the WebAssembly module to.
//  var g_WebWorker = new Worker("webworker.js");
//  g_WebWorker.onerror = function (evt) { console.log(`Error from Web Worker: ${evt.message}`); }
//  g_WebWorker.onmessage = function (evt) { alert(`Message from the Web Worker:\n\n ${evt.data}`); }
//  // Request the wasm file from the server and compile it...(Typically we would call
//  // 'WebAssembly.instantiate' which compiles and instantiates the module. In this
//  // case, however, we just want the compiled module which will be passed to the Web
//  // Worker. The Web Worker will be responsible for instantiating the module.)
//  fetch("mmix.wasm").then(response =>
//    response.arrayBuffer()
//  ).then(bytes =>
//    WebAssembly.compile(bytes)
//  ).then(WasmModule =>
//    g_WebWorker.postMessage({ "MessagePurpose": "CompiledModule", "WasmModule": WasmModule })
//  );
//  function OnClickTest() {
//    // Ask the Web Worker to add two values
//    g_WebWorker.postMessage({ "MessagePurpose": "AddValues", "Val1": 1, "Val2": 2 });
//  }
