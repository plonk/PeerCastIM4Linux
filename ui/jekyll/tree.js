var start_time1 = new Date();

var agt=navigator.userAgent.toLowerCase();
var is_opera = (agt.indexOf("opera") != -1);
var is_firefox = (agt.indexOf("firefox") != -1);

var i, j;

var color_table = new Array('#000000','#2f4f4f', '#191970', '#3000e0', '#3400e0', '#3700e0', '#3b00e0', '#3f00e0', '#4300e0', '#4600e0', '#4a00e0', '#4e00e0', '#5200e0', '#5500e0', '#5900e0', '#5d00e0', '#6100e0', '#6400e0', '#6800e0', '#6c00e0', '#7000e0', '#7300e0', '#7700e0', '#7b00e0', '#7e00e0', '#8200e0', '#8600e0', '#8a00e0', '#8d00e0', '#9100e0', '#9500e0', '#9900e0', '#9c00e0', '#a000e0', '#a400e0', '#a800e0', '#ab00e0', '#af00e0', '#b300e0', '#b700e0', '#ba00e0', '#be00e0', '#c200e0', '#c600e0', '#c900e0', '#cd00e0', '#d100e0', '#d500e0', '#d800e0', '#dc00e0', '#e000e0', '#e000dc', '#e000d8', '#e000d5', '#e000d1', '#e000cd', '#e000c9', '#e000c6', '#e000c2', '#e000be', '#e000ba', '#e000b7', '#e000b3', '#e000af', '#e000ab', '#e000a8', '#e000a4', '#e000a0', '#e0009c', '#e00099', '#e00095', '#e00091', '#e0008d', '#e0008a', '#e00086', '#e00082', '#e0007e', '#e0007b', '#e00077', '#e00073', '#e00070', '#e0006c', '#e00068', '#e00064', '#e00061', '#e0005d', '#e00059', '#e00055', '#e00052', '#e0004e', '#e0004a', '#e00046', '#e00043', '#e0003f', '#e0003b', '#e00037', '#e00034', '#e00030', '#e0002c', '#e00028', '#b8860b');

var hosts = new Array();
var ref = new Array();
var tree = new Array();
var hosts_count = 0;

var lines = document.data.hosts.value.split('||');



for(i=0; i<lines.length; i++)
{	// ホストリスト・ツリー作成用テーブル・ツリーの素作成
	var items = lines[i].split('|');

	if(items.length != 8 || items[0].indexOf('loop.') != -1)
		continue;

	temp2 = items[0].match(/([\d]+\.[\d]+\.[\d]+\.[\d]+:[\d]+)(\[([^\]]+)\])?/);

	var temp = new Array();
	temp['ip'] = temp2[1];
	temp['_ip'] = get_ip(temp2[1]);
	temp['name'] = temp2[3];
	temp['upip'] = items[1];
	temp['_upip'] = get_ip(items[1]);
	temp['hops'] = items[2];
	temp['relays'] = items[3];
	temp['canrelay'] = items[4];
	temp['push'] = items[5];
	temp['uptime'] = get_time(String(items[6]));
	temp['agent'] = items[7];
	temp['allow'] = 0;
	temp['global_upip'] = is_global(temp['upip']);


	if(temp['hops'] == 1)
		tree[temp['ip']] = temp['ip'];

	if(temp['hops'] > 0)
	{
		hosts[temp['ip']] = temp;
		hosts_count++;
	}

	if(temp['hops'] > 1 && temp['global_upip'])
	{
		if(ref[temp['upip']] == undefined)
			ref[temp['upip']] = new Array();

		ref[temp['upip']][temp['ip']] = temp['ip'];
	}
}



for(var ip in hosts)
{
	// ローカルでリレーしているホストの処理
	if(hosts[ip]['hops'] > 1 && !hosts[ip]['global_upip'])
	{
		var newip = get_ip(ip) + ':' + get_port(hosts[ip]['upip']);

		if(ref[newip] == undefined)
			ref[newip] = new Array();

		ref[newip][ip] = ip;
		hosts[ip]['upip'] = newip;
	}

	// ポート0でリレーしているホストの処理
	if(hosts[ip]['push'] == 1 && hosts[ip]['relays'] > 0)
	{
		for(var _ip in hosts)
		{
			if(hosts[ip]['_ip'] == hosts[_ip]['_upip'] && ip != _ip)
			{
				if(ref[ip] == undefined)
					ref[ip] = new Array();

				ref[ip][_ip] = _ip;
				hosts[_ip]['upip'] = ip;
			}
		}
	}
}



function create_tree(tree, depth)
{	// ツリー作成
	var i;
	var ret = new Array();

	if(depth > 100)
		alert('Excursion create_tree(2)');

	ret['relays_total'] = 0;

	for(var ip in tree)
	{
		if(hosts[ip]['allow'] == 0)
		{
			hosts[ip]['allow'] = 1;

			if(ref[ip] != undefined)
			{
				var res;

				tree[ip] = ref[ip];
				res = create_tree(tree[ip], depth+1);

				ret['relays_total'] += res['relays_total'];

				array_merge(hosts[ip], res);
			}
			else
			{
				var res = new Array();

				res['relays_total'] = 0;

				array_merge(hosts[ip], res);
			}

			ret['relays_total']++;
		}
	}

	return ret;
}

create_tree(tree, 1);



var tree_id = 1;

function print_tree(tree, depth, lin)
{	// ツリー表示
	var i, j;
	var count;

	if(depth > 100)
		alert('Excursion print_tree(3)');

	count = 0;
	for(var ip in tree)
		count++;

	i = 0;
	for(var ip in tree)
	{
		var out_temp = '';

		for(j=0; j<depth; j++)
		{
			if(lin[j] != undefined)
				out_temp += '┃';
			else
				out_temp += '  ';
		}

		var flag = depth > 0 && hosts[hosts[ip]['upip']] != undefined && hosts[ip]['hops']-1 != hosts[hosts[ip]['upip']]['hops'];
		if(flag)	// ツリーがなんかおかしい
			out_temp += '<font color="#ee0000">';

		if(i == count-1)
			out_temp += '┗';
		else
			out_temp += '┣';

		var offset = 0;

		if(flag)
		{
			var k;

			for(k=depth+1; k<hosts[ip]['hops']; k++, offset++)
				out_temp += '━';

			out_temp += '</font>';
		}

		out_temp += "<a href=\"JavaScript:deproy_some_tree(" + tree_id + "," + hosts[ip]['relays_total'] + ")\">";

		if(typeof(tree[ip]) == 'object')
			out_temp += '<font color="' + get_relay_color(hosts[ip]) + '">■</font></a> ';
		else
			out_temp += '<font color="' + get_relay_color(hosts[ip]) + '">●</font></a> ';

		out_temp += "<a href=\"JavaScript:deproy_tree(" + tree_id + ")\">";
		out_temp += '<font color="' + color_table[get_time_color(hosts[ip]['uptime'])] + '">' + ip + '</font> ';
		out_temp += '<font color="#000000">(' + hosts[ip]['relays_total'] + '/' + hosts[ip]['relays'] + ') [' + hosts[ip]['hops'] + ']</font> ';

		if(hosts[ip]['name'] != undefined && hosts[ip]['name'] != '')
			out_temp += '<font color="#888888">[' + hosts[ip]['name'] + ']</font> ';

		out_temp += '<font color="#888888">' + hosts[ip]['agent'] + "</font></a><a href=\"#\" onclick=\"checkip('" + get_ip(ip) + "')\">_</a>\n";

		out_temp += '<div id="tree' + tree_id + '" style="display:block;">';
		tree_id++;

		if(is_opera || is_firefox)
			out += out_temp;
		else
			document.write(out_temp);

		if(typeof(tree[ip]) == 'object')
		{
			if(count > 1 && i < count-1)
				lin[depth] = depth;

			print_tree(tree[ip], depth+1+offset, lin);

			delete lin[depth];
		}

		if(is_opera || is_firefox)
			out += '</div>';
		else
			document.write('</div>');

		i++;
	}
}

var temp = 0;
for(var ip in tree)
	temp++;

document.write("<a href=\"JavaScript:deproy_some_tree(" + tree_id + "," + (hosts_count-1) + ")\">");
document.write("<font color=\"#000000\">■</font></a> (" + hosts_count + '/' + temp + ") [0]\n");

if(is_opera || is_firefox)
{
	var out = '';
	print_tree(tree, 0, new Array());
	document.write(out);
}
else
{
	print_tree(tree, 0, new Array());
}




var umhosts = new Array();
var max_hops = 0;

for(var ip in hosts)
{	// ツリー化不可ホストリスト作成
	if(hosts[ip]['allow'] == 0)
	{
		if(umhosts[hosts[ip]['hops']] == undefined)
			umhosts[hosts[ip]['hops']] = new Array();

		umhosts[hosts[ip]['hops']][ip] = ip;

		max_hops = Math.max(max_hops, hosts[ip]['hops']);
	}
}



document.write("\n\n");

for(i=0; i<=max_hops; i++)
{	// ツリー化不可ホスト表示
	if(umhosts[i] != undefined)
	{
		for(var ip in umhosts[i])
		{
			if(hosts[ip]['allow'] == 0)
			{
				var umtree = new Array();
				var hops = hosts[ip]['hops']-1;

				for(j=0; j<hops; j++)
					document.write('  ');
				document.write('<font color="#888888">■ ' + hosts[ip]['upip'] + "</font>\n");

				if(ref[hosts[ip]['upip']] != undefined)
					umtree = ref[hosts[ip]['upip']];
				else
					umtree[ip] = ip;

				create_tree(umtree, hops);

				if(is_opera || is_firefox)
				{
					var out = '';
					print_tree(umtree, hops, new Array());
					document.write(out);
				}
				else
				{
					print_tree(umtree, hops, new Array());
				}

				document.write("\n");
			}
		}
	}
}

//生成時間計測
var end_time = new Date();
document.write('</pre><br>Generate time: ' + parseInt(end_time  - start_time1) + ' ms<br>');

function get_time(str)
{
	var ret = 0;
	var temp = str.match(/(([\d]+) day)?[^\d]*(([\d]+) hour)?[^\d]*(([\d]+) min)?[^\d]*(([\d]+) sec)?/);

	if(temp[2] != undefined && temp[2] != '') ret += parseInt(temp[2]) * 60*60*24;
	if(temp[4] != undefined && temp[4] != '') ret += parseInt(temp[4]) * 60*60;
	if(temp[6] != undefined && temp[6] != '') ret += parseInt(temp[6]) * 60;
	if(temp[8] != undefined && temp[8] != '') ret += parseInt(temp[8]);

	return ret;
}

function get_time_color(timestr)
{
	var colortemp = 0;
	if(timestr <= 3600){// 60分以下
		colortemp = Math.round(timestr / 360);
	}else if(timestr > 3600 && timestr <= 43200){
		colortemp = Math.ceil(timestr / 432);
		if(colortemp < 10){
			colortemp = 10;
		}
	}else if(timestr > 43200){// 12時間以上
		colortemp = 100;
	}

	return colortemp;
}

function get_relay_color(host)
{
	if(host['push'] == 1)
		if(host['relays'] == 0)
			return '#ff0000';
	else
		return '#ffa500';
	else
		if(host['canrelay'] == 0)
			if(host['relays'] == 0)
				return '#9400d3';
	else
		return '#0000ff';
	else
		return '#00cc00';
}

function is_global(ip)
{
	var temp = ip.match(/([\d]+)\.([\d]+)\.([\d]+)\.([\d]+)/);

	temp[1] = parseInt(temp[1]);
	temp[2] = parseInt(temp[2]);
	temp[3] = parseInt(temp[3]);
	temp[4] = parseInt(temp[4]);

	if(temp[1] == 192 && temp[2] == 168)
		return false;
	if(temp[1] == 127 && temp[2] == 0 && temp[3] == 0 && temp[4] == 1)
		return false;
	if(temp[1] == 172 && temp[2] >= 16 && temp[2] <= 31)
		return false;
	if(temp[1] == 10)
		return false;

	return true;
}

function get_ip(ip)
{
    //	var temp = ip.match(/([\d]+\.[\d]+\.[\d]+\.[\d]+)/);
	var temp = ip.match(/([^:]+)/);
	return temp[1];
}

function get_port(ip)
{
    //	var temp = ip.match(/[^:]+:([\d]+)/);
	var temp = ip.match(/:([\d]+)/);
	return temp[1];
}

function array_merge(dst, src)
{
	for(var key in src)
	{
		dst[key] = src[key];
	}
}

function deproy(id)
{
	if(document.all(id).style.display == "none")
		document.all(id).style.display = "block";
	else
		document.all(id).style.display = "none";
}

function deproy_tree(id)
{
	id = 'tree' + id;

	if(document.all(id).style.display == "none")
		document.all(id).style.display = "block";
	else
		document.all(id).style.display = "none";
}

function deproy_some_tree(start, num)
{
	var i;
	var state;
	var end = start + num;
	var id = 'tree' + start;

	if(document.all(id).style.display == "none")
		state = "block";
	else
		state = "none";

	for(i=start; i<=end; i++)
	{
		id = 'tree' + String(i);

		document.all(id).style.display = state;
	}
}

function checkip(ip)
{
	document.check.ip.value = ip;
	document.check.submit();
}
// Author: ◆e5bW6vDOJ.
