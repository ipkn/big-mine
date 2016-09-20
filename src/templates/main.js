var BORDER_DEBUG = 0

var color_table = [
    "rgba(0,0,0,0)",
    "rgba(0,0,192,0.5)",
    "rgba(0,192,0,0.5)",
    "rgba(192,0,0,0.5)",
    "rgba(0,0,128,1)",
    "rgba(128,128,0,1)",
    "rgba(0,128,128,1)",
    "rgba(0,0,0,1)",
    "rgba(128,128,128,1)",
];
var flag_glyph = [
"   b   ",
" bbb   ",
"bbbb   ",
" bbb   ",
"   b   ",
"   a   ",
" aaaaa ",
];
var font = [
"*** * ******* *****  *********",
"* * *   *  ** **  *    ** ** *",
"* * * ***************  *******",
"* * * *    *  *  ** *  ** *  *",
"*** * ******  *******  ****  *",
];
var state = [0,1,2,3,4,5,6,7,8,'normal','debris','flagged'];
var ws = new WebSocket('ws://' + location.host + '/ws');
var playground = document.getElementById('playground');
var fields = {};
var mx = -100;
var my = -100;
var tile_size = 32;
var field_size = 20;
var sx = 0;
var sy = 0;
function Field(x, y, data) {
    this.x = x;
    this.y = y;
    this.raw = data;
    fields[x+"_"+y] = this;
    this.div = document.createElement('div');
    this.div.className = "cell";
    this.div.id = "div_"+x+"_"+y;
    this.div.style.left = x*tile_size*field_size + "px";
    this.div.style.top = y*tile_size*field_size + "px";
    this.div.style.width = field_size*tile_size + "px";
    this.div.style.height = field_size*tile_size + "px";

    if (BORDER_DEBUG)
    if( (x+y)%2 == 1) this.div.style.backgroundColor = 'red'; else this.div.style.backgroundColor = 'blue';

    this.canvas = document.createElement('canvas');
    this.canvas.style.width = field_size*tile_size + "px";
    this.canvas.style.height = field_size*tile_size + "px";
    this.canvas.style.left = "0";
    this.canvas.style.top = "0";
    this.canvas.width = tile_size*field_size;
    this.canvas.height = tile_size*field_size;
    this.canvas.className = "cell_canvas";
    this.ctx = this.canvas.getContext('2d');
    this.div.appendChild(this.canvas);
    playground.appendChild(this.div);
    this.render_needed = true;
    this.render();
}

function dist2(x1,y1,x2,y2)
{
    return (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
}

function get_at(fx, fy, x, y) {
    if (x < 0) return get_at(fx-1,fy,x+field_size,y);
    if (y < 0) return get_at(fx,fy-1,x,y+field_size);
    if (x >= field_size) return get_at(fx+1,fy,x-field_size,y);
    if (y >= field_size) return get_at(fx,fy+1,x,y-field_size);
    if (fx+"_"+fy in fields)
        return fields[fx+"_"+fy].raw[x+y*field_size];
    return -1;
}
function get_around(fx, fy, x, y, c) {
    return (
        (get_at(fx,fy,x-1,y-1) == c ? 1 : 0) +
        (get_at(fx,fy,x-1,y  ) == c ? 1 : 0) +
        (get_at(fx,fy,x-1,y+1) == c ? 1 : 0) +
        (get_at(fx,fy,x  ,y-1) == c ? 1 : 0) +
        (get_at(fx,fy,x  ,y+1) == c ? 1 : 0) +
        (get_at(fx,fy,x+1,y-1) == c ? 1 : 0) +
        (get_at(fx,fy,x+1,y  ) == c ? 1 : 0) +
        (get_at(fx,fy,x+1,y+1) == c ? 1 : 0));
}

Field.prototype.open = function(x, y) {
    if (this.raw[y*field_size+x] == 9) {
        ws.send("0 "+this.x + " " + this.y + " " + x+" "+y + " 0");
        console.log("S");
    } else if (this.raw[y*field_size+x] <= 8 && this.raw[y*field_size+x] >= 1) {
        var n_closed = get_around(this.x, this.y, x, y, 9);
        var n_flag = get_around(this.x, this.y, x, y, 10);
        console.log('wo', n_closed, n_flag);
        if (n_closed > 0 && n_flag == this.raw[y*field_size+x]) {
            ws.send("0 "+this.x + " " + this.y + " " + x+" "+y + " 1");
        }
    }
}

Field.prototype.flag = function(x, y) {
    console.log(this.raw[y*field_size+x]);
    if (this.raw[y*field_size+x] == 9 ||
        this.raw[y*field_size+x] == 10)
    {
        ws.send("1 "+this.x + " " + this.y + " " + x+" "+y+(this.raw[y*field_size+x]==10?" 0 0":" 1 0"));
    } else if (this.raw[y*field_size+x] <= 8 && this.raw[y*field_size+x] >= 1) {
        var n_closed = get_around(this.x, this.y, x, y, 9);
        var n_flag = get_around(this.x, this.y, x, y, 10);
        console.log('wf', n_closed, n_flag);
        if ((n_closed+n_flag) == this.raw[y*field_size+x] && n_closed > 0) {
            ws.send("1 "+this.x + " " + this.y + " " + x+" "+y+" 1 1");
        }
    }
}

Field.prototype.update = function(data) {
    this.raw = data;
    this.render_needed = true;
}

Field.prototype.render = function() {
    if (!this.render_needed)
        return;
    var ctx = this.ctx;
    var t = Date.now();
    this.render_needed = false;
    var idx = 0;
    for(var y = 0; y < field_size; y++)
    for(var x = 0; x < field_size; x++)
    {
        var cx = x*tile_size+tile_size/2+this.x*tile_size*field_size;
        var cy = y*tile_size+tile_size/2+this.y*tile_size*field_size;
        var c = 185+(x*this.x*y*this.y+x*x+x*y+3*x+7*y+10*this.x+4*this.y)%11;
        //if (dist2(mx,my,cx,cy) < tile_size*tile_size*8) {
            //c = Math.floor(Math.random() * 15-7 + 200);
        //}
        if (this.raw[idx] == 9)
        {
            ctx.fillStyle = "rgb("+c+","+c+","+c+")";
            ctx.fillRect(x*tile_size,y*tile_size,tile_size-2,tile_size-2);
        }
        else if (this.raw[idx] == 0)
        {
            ctx.fillStyle = "rgb(255, 255, 255)"
            ctx.fillRect(x*tile_size,y*tile_size,tile_size-2,tile_size-2);
        }
        else if (this.raw[idx] <= 8)
        {
            ctx.fillStyle = "rgb(255, 255, 255)"
            ctx.fillRect(x*tile_size,y*tile_size,tile_size-2,tile_size-2);
            ctx.fillStyle = color_table[this.raw[idx]];
            for(var i = 0; i < 3; i++)
                for(var j = 0; j < 5; j ++) {
                    
                    if (font[j][i+this.raw[idx]*3]=='*') {
                        ctx.fillRect(x*tile_size+i*3+15-5, y*tile_size+j*3+15-7, 3, 3);
                    }
                }

        } else if (this.raw[idx] == 10) {
            // flag
            ctx.fillStyle = "rgb("+c+","+c+","+c+")";
            ctx.fillRect(x*tile_size,y*tile_size,tile_size-2,tile_size-2);
            for(var i = 0; i < 7; i++)
                for(var j = 0; j < 7; j ++) {
                    if (flag_glyph[j][i]=='a') {
                        ctx.fillStyle = "black";
                        ctx.fillRect(x*tile_size+i*3+15-9, y*tile_size+j*3+15-9, 3, 3);
                    } else if (flag_glyph[j][i]=='b') {
                        ctx.fillStyle = "red";
                        ctx.fillRect(x*tile_size+i*3+15-9, y*tile_size+j*3+15-9, 3, 3);
                    }
                }
        } else if (this.raw[idx] == 11) {
            // bomb
        } else if (this.raw[idx] == 12) {
            // debris
        }
        if (cx-tile_size/2 <= mx && mx < cx+tile_size/2 && cy-tile_size/2 <= my && my < cy+tile_size/2) {
            ctx.strokeStyle = "rgb("+50+","+50+","+50+")";
            ctx.strokeRect(x*tile_size+2,y*tile_size+2,tile_size-6,tile_size-6);
            //ctx.fillStyle = "rgb("+50+","+50+","+50+")";
            //ctx.fillRect(x*tile_size,y*tile_size,tile_size-2,tile_size-2);
            //ctx.fillStyle = "rgb("+c+","+c+","+c+")";
            //ctx.fillRect(x*tile_size+2,y*tile_size+2,tile_size-6,tile_size-6);
            this.render_needed = true;
        }
        idx ++;
    }
}

ws.binaryType = "arraybuffer";
ws.onclose = function(e) {
    ws = new WebSocket('ws://' + location.host + '/ws');
    ws.binaryType = "arraybuffer";
}
ws.onmessage = function(e){
    if (typeof e.data == "string") {
        var json = JSON.parse(e.data);
        if (json.m == "see") {
            console.log(json);
            sx = window.innerWidth/2-json.x*tile_size;
            sy = window.innerHeight/2-json.y*tile_size;
            playground.style.transform = "translate("+(window.innerWidth/2-json.x*tile_size)+"px,"+(window.innerHeight/2-json.y*tile_size)+"px)";
        }
    } else {
        var data = pako.inflate(e.data);
        x = data[data.length-4]+data[data.length-3] * 256;
        y = data[data.length-2]+data[data.length-1] * 256;
        data = data.slice(0, data.length-4);
        if (x+"_"+y in fields)
            fields[x+"_"+y].update(data);
        else
            new Field(x,y,data);
    }
};
var onEachFrame;
if (window.requestAnimationFrame) {
  onEachFrame = function(cb) {
    var _cb,
      _this = this;
    _cb = function() {
      cb();
      return window.requestAnimationFrame(_cb);
    };
    return _cb();
  };
} else if (window.webkitRequestAnimationFrame) {
  onEachFrame = function(cb) {
    var _cb,
      _this = this;
    _cb = function() {
      cb();
      return window.webkitRequestAnimationFrame(_cb);
    };
    return _cb();
  };
} else if (window.mozRequestAnimationFrame) {
  onEachFrame = function(cb) {
    var _cb,
      _this = this;
    _cb = function() {
      cb();
      return window.mozRequestAnimationFrame(_cb);
    };
    return _cb();
  };
} else {
  onEachFrame = function(cb) {
    return setInterval(cb, 1000 / 60);
  };
}

onEachFrame(function(){
    //context.beginPath();
    //f(force_redraw);
    //context.stroke();
    for(var key in fields)
    {
        fields[key].render();
    }
});

document.addEventListener('mousemove', function(e){
    mx = e.pageX-sx;
    my = e.pageY-sy;
    var cx = Math.floor(mx / tile_size/field_size);
    var cy = Math.floor(my / tile_size/field_size);
    if (cx+"_"+cy in fields) {
        fields[cx+"_"+cy].render_needed = true;
    }
});
var mbeginx = -100;
var mbeginy = -100;
var mbeginsx = -1;
var mbeginsy = -1;
var mcurfx = -1;
var mcurfy = -1;
var ondrag_flag = false;
document.oncontextmenu = function(e) {
    e.preventDefault();
    return false;
}
document.addEventListener('mousedown', function(e) {
    e.preventDefault();
    if (e.which == 1)
        ondrag_flag = true;
    mbeginx = e.pageX;
    mbeginy = e.pageY;
    mbeginsx = sx;
    mbeginsy = sy;
    mcurfx = Math.floor((-sx + window.innerWidth/2)/tile_size/field_size);
    mcurfy = Math.floor((-sy + window.innerHeight/2)/tile_size/field_size);
});
document.addEventListener('mousemove', function(e) {
    e.preventDefault();
    if (!ondrag_flag)
        return;
    mx = e.pageX-sx;
    my = e.pageY-sy;
    e.pageX;
    e.pageY;
    if (Math.abs(e.pageX-mbeginx) + Math.abs(e.pageY-mbeginy) > 5) {
        sx = mbeginsx + e.pageX - mbeginx;
        sy = mbeginsy + e.pageY - mbeginy;
        playground.style.transform = "translate("+sx+"px,"+sy+"px)";
        var fx = Math.floor((-sx + window.innerWidth/2)/tile_size/field_size);
        var fy = Math.floor((-sy + window.innerHeight/2)/tile_size/field_size);
        if (fx != mcurfx || fy != mcurfy)
        {
            ws.send("2 " + Math.floor((-sx + window.innerWidth/2)/tile_size) + ' ' + Math.floor((-sy + window.innerHeight/2)/tile_size) );
            mcurfx = fx;
            mcurfy = fy;
        }
    }
});
document.addEventListener('mouseup', function(e) {
    ondrag_flag = false;
    e.preventDefault();
    mx = e.pageX-sx;
    my = e.pageY-sy;
    if (Math.abs(mbeginx - e.pageX) + Math.abs(mbeginy - e.pageY)<7) {
        var cx = Math.floor(mx / tile_size/field_size);
        var cy = Math.floor(my / tile_size/field_size);
        if (cx+"_"+cy in fields) {
            if (e.which == 1) {
                fields[cx+"_"+cy].open(Math.floor((mx-cx*tile_size*field_size)/tile_size), Math.floor((my-cy*tile_size*field_size)/tile_size));
            } else if (e.which == 3) {
                fields[cx+"_"+cy].flag(Math.floor((mx-cx*tile_size*field_size)/tile_size), Math.floor((my-cy*tile_size*field_size)/tile_size));
            }
        }
    }
});
/*
document.addEventListener('click', function(e) {
    e.preventDefault();
    mx = e.pageX-sx;
    my = e.pageY-sy;
    if (mbeginx == e.pageX && mbeginy == e.pageY) {
        var cx = Math.floor(mx / tile_size/field_size);
        var cy = Math.floor(my / tile_size/field_size);
        if (cx+"_"+cy in fields) {
            fields[cx+"_"+cy].open(Math.floor((mx-cx*tile_size*field_size)/tile_size), Math.floor((my-cy*tile_size*field_size)/tile_size));
        }
    }
});
document.addEventListener('tap', function(e) {
    mx = e.pageX-sx;
    my = e.pageY-sy;
    var cx = Math.floor(mx / tile_size/field_size);
    var cy = Math.floor(my / tile_size/field_size);
    if (cx+"_"+cy in fields) {
        fields[cx+"_"+cy].open(Math.floor((mx-cx*tile_size*field_size)/tile_size), Math.floor((my-cy*tile_size*field_size)/tile_size));
    }
});
*/
