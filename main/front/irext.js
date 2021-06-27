/*
 * @Author: Caffreyfans
 * @Date: 2021-06-27 15:46:46
 * @LastEditTime: 2021-06-27 22:37:14
 * @Description: 
 */
var XMLHttpRequest = require("xmlhttprequest").XMLHttpRequest;

var irext_url_prefix = "https://irext.net/irext-server"
function irext_login(app_key, app_secret) {
	var token = {}
	var irext_url_suffix = "/app/app_login";
	var url = irext_url_prefix + irext_url_suffix;
	var body = {
		"appKey": app_key,
		"appSecret": app_secret,
		"appType": "2"
	};
	var xhr = new XMLHttpRequest();
	console.log("url = " + url);
	xhr.open("POST", url, false);
	xhr.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
	xhr.onload = function (e) {
		if (this.status == 200) {
			var ret = JSON.parse(this.responseText);
			token["token"] = ret["entity"]["token"];
			token["id"] = ret["entity"]["id"];
		}
	};
	xhr.send(JSON.stringify(body));
	return token;
}

function irext_list_brands(token, categoryId) {
	var list = new Array();
	var irext_url_suffix = "/indexing/list_brands";
	var url = irext_url_prefix + irext_url_suffix;
	var body = token;
	body["from"] = "0";
	body["count"] = "1000";
	body["categoryId"] = categoryId;
	var xhr = new XMLHttpRequest();
	xhr.open("POST", url, false);
	xhr.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
	xhr.onload = function (e) {
		if (this.status = 200) {
			var ret = JSON.parse(this.responseText);
			ret = ret["entity"];
			for (var item of ret) {
				var tmp = {};
				tmp["id"] = item["id"];
				tmp["categoryId"] = item["categoryId"];
				tmp["name"] = item["name"];
				list.push(tmp);
			}
		}
	}
	xhr.send(JSON.stringify(body));
	return list;
}

function irext_list_indexes(token, categoryId, brandId) {
	var list = new Array();
	var irext_url_suffix = "/indexing/list_indexes";
	var url = irext_url_prefix + irext_url_suffix;
	var body = token;
	body["from"] = "0";
	body["count"] = "1000";
	body["categoryId"] = categoryId;
	body["brandId"] = brandId;
	var xhr = new XMLHttpRequest();
	xhr.open("POST", url, false);
	xhr.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
	xhr.onload = function (e) {
		if (this.status = 200) {
			var ret = JSON.parse(this.responseText);
			ret = ret["entity"];
			for (var item of ret) {
				var tmp = {};
				tmp["remoteMap"] = item["remoteMap"];
				list.push(tmp);
			}
		}
	}
	xhr.send(JSON.stringify(body));
	return list;
}
var app_key = 'cdf33048c9dbef2962b0f915bc7e420c';
var app_secret = 'f00f57af376c66ca1355cfe109400dd2';
var token = irext_login(app_key, app_secret);
var list_brands = irext_list_brands(token, "1");
console.log(list_brands);