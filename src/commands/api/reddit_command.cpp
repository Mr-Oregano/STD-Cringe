/*
 * MIT License
 *
 * Copyright (c) 2023 @nulzo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <dpp/dpp.h>
#include <fmt/format.h>
#include "utils/embed/cringe_embed.h"
#include "utils/http/cringe_curl.h"
#include "commands/api/reddit_command.h"
#include "utils/misc/cringe_color.h"
#include "utils/misc/cringe_icon.h"
#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <random>

dpp::slashcommand reddit_declaration() {
	return dpp::slashcommand()
			.set_name("reddit")
			.set_description("Get the top post from a subreddit")
			.add_option(dpp::command_option(dpp::co_string, "subreddit",
											"Subreddit to get post from", true))
			.add_option(
					dpp::command_option(dpp::co_string, "top", "Get the top post of timeframe", true)
							.add_choice(dpp::command_option_choice("Day", std::string("day")))
							.add_choice(dpp::command_option_choice("Month", std::string("month")))
							.add_choice(dpp::command_option_choice("Year", std::string("year")))
							.add_choice(dpp::command_option_choice("All-Time", std::string("all")))
							.add_choice(dpp::command_option_choice("Random", std::string("random"))));
}

std::string make_gif(std::string url) {
	// Allocate c style buf to store result of command
	char buffer[128];
	std::string gif;
	// string that gets all the information about the song
	std::string cmd = fmt::format("yt-dlp -f \"w\" -o - \"{}\" | ffmpeg -i pipe:0 -vf \"fps=10,scale=320:-1:flags=lanczos\" -c:v pam -f image2pipe -", url);
	// Get the YouTube video thumbnail, author info, and song info
	FILE *pipe = popen(cmd.c_str(), "r");
	// Check that the pipe was opened successfully
	if (!pipe) {
		std::cerr << "Error opening pipe" << std::endl;
	}
	// Write contents of stdout buf to c++ style string
	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
		gif += buffer;
	}
	return gif;
}

dpp::embed reddit_embed(json data) {
	std::string author = data["author"];
	std::string subreddit = data["subreddit"];
	std::string title = data["title"];
	std::string description = data["description"];
	int creation = data["time_created"];
	std::string comments = to_string(data["comments"]);
	std::string upvotes = to_string(data["upvotes"]);
	auto media = data["media"];
	auto e = data["media_embed"];
	float ratio = data["upvote_ratio"];
	auto video = data["video"];

	dpp::embed embed;

	embed.set_color(CringeColor::CringePrimary)
			.set_title("Cringe Crawler")
			.set_thumbnail(CringeIcon::LightningIcon)
			.add_field("", "+-----------------------------------------------------------------+")
			.add_field("Subreddit", subreddit, true)
			.add_field("Author", author, true)
			.add_field("Posted", fmt::format("<t:{}:D>", creation), true)
			.add_field("Comments", comments, true)
			.add_field("Upvotes", upvotes, true)
			.add_field("Upvote Ratio", fmt::format("{}%", ratio * 100), true)
			.add_field("", "+-----------------------------------------------------------------+");

	if (!title.empty()) {
		embed.add_field("Title", title);
		embed.add_field("", "+-----------------------------------------------------------------+");
	}

	if (!description.empty()) {
		embed.add_field("Description", description);
		embed.add_field("", "+-----------------------------------------------------------------+");
	}

	embed.set_timestamp(time(nullptr));

	if (!media.empty() && media.contains("oembed")) {
		if (media["oembed"].contains("thumbnail_url")) {
			embed.set_image(media["oembed"]["thumbnail_url"]);
		}
	}

	if (!video.empty()) {
		embed.set_video(video);
		embed.add_field("Video", video);
	}

	return embed;
}

json parse_reddit_response(json res) {
	json pre_response;
	json response;
	pre_response = res;
	pre_response = pre_response["data"]["children"][0]["data"];
	response["subreddit"] = pre_response["subreddit_name_prefixed"];
	response["title"] = pre_response["title"];
	response["author"] = pre_response["author"];
	response["description"] = pre_response["selftext"];
	response["upvote_ratio"] = pre_response["upvote_ratio"];
	response["time_created"] = pre_response["created"];
	response["url"] = pre_response["url"];
	response["media"] = pre_response["media"];
	response["embed"] = pre_response["media_embed"];
	response["media_embed"] = pre_response["secure_media_embed"];
	response["upvotes"] = pre_response["ups"];
	response["comments"] = pre_response["num_comments"];
	response["thumbnail"] = pre_response["thumbnail"];
	response["video"] = pre_response["preview"]["reddit_video_preview"]["fallback_url"];

	return response;
}

const std::vector<std::string> SUBREDDITS = {
		"ListOfSubreddits",
		"ListOfSubreddits/wiki/nsfw/",
		"NSFW411",
		"nsfw",
		"nsfw2",
		"bonermaterial",
		"nsfw411",
		"iWantToFuckHer",
		"exxxtras",
		"distension",
		"bimbofetish",
		"christiangirls",
		"dirtygaming",
		"sexybutnotporn",
		"femalepov",
		"omgbeckylookathiscock",
		"sexygirls",
		"breedingmaterial",
		"canthold",
		"toocuteforporn",
		"justhotwomen",
		"stripgirls",
		"hotstuffnsfw",
		"uncommonposes",
		"gifsofremoval",
		"nostalgiafapping",
		"truefmk",
		"nudes",
		"slut",
		"TipOfMyPenis",
		"pornID",
		"milf",
		"gonewild30plus",
		"preggoporn",
		"realmoms",
		"agedbeauty",
		"40plusgonewild",
		"maturemilf",
		"legalteens",
		"collegesluts",
		"adorableporn",
		"legalteensXXX",
		"gonewild18",
		"18_19",
		"just18",
		"PornStarletHQ",
		"fauxbait",
		"barelylegalteens",
		"realgirls",
		"amateur",
		"homemadexxx",
		"dirtypenpals",
		"FestivalSluts",
		"CollegeAmateurs",
		"amateurcumsluts",
		"nsfw_amateurs",
		"funwithfriends",
		"randomsexiness",
		"amateurporn",
		"normalnudes",
		"ItsAmateurHour",
		"irlgirls",
		"verifiedamateurs",
		"NSFWverifiedamateurs",
		"Camwhores",
		"camsluts",
		"streamersgonewild",
		"GoneWild",
		"PetiteGoneWild",
		"gonewildstories",
		"GoneWildTube",
		"treesgonewild",
		"gonewildaudio",
		"GWNerdy",
		"gonemild",
		"altgonewild",
		"gifsgonewild",
		"analgw",
		"gonewildsmiles",
		"onstageGW",
		"RepressedGoneWild",
		"bdsmgw",
		"UnderwearGW",
		"LabiaGW",
		"TributeMe",
		"WeddingsGoneWild",
		"gwpublic",
		"assholegonewild",
		"leggingsgonewild",
		"dykesgonewild",
		"goneerotic",
		"snapchatgw",
		"gonewildhairy",
		"gonewildtrans",
		"gonwild",
		"ratemynudebody",
		"gonewild30plus",
		"gonewild18",
		"onmww",
		"ohnomomwentwild",
		"40plusgonewild",
		"GWCouples",
		"gonewildcouples",
		"gwcumsluts",
		"WouldYouFuckMyWife",
		"couplesgonewild",
		"gonewildcurvy",
		"GoneWildplus",
		"BigBoobsGW",
		"bigboobsgonewild",
		"mycleavage",
		"gonewildchubby",
		"AsiansGoneWild",
		"gonewildcolor",
		"indiansgonewild",
		"latinasgw",
		"pawgtastic",
		"workgonewild",
		"GoneWildScrubs",
		"swingersgw",
		"militarygonewild",
		"NSFW_Snapchat",
		"snapchat_sluts",
		"snapleaks",
		"wifesharing",
		"hotwife",
		"wouldyoufuckmywife",
		"slutwife",
		"naughtywives",
		"rule34",
		"ecchi",
		"futanari",
		"doujinshi",
		"yiff",
		"furry",
		"monstergirl",
		"rule34_comics",
		"sex_comics",
		"hentai",
		"hentai_gif",
		"WesternHentai",
		"hentai_irl",
		"traphentai",
		"hentaibondage",
		"overwatch_porn",
		"pokeporn",
		"bowsette",
		"rule34lol",
		"rule34overwatch",
		"BDSM",
		"Bondage",
		"BDSMcommunity",
		"bdsmgw",
		"femdom",
		"blowjobs",
		"lipsthatgrip",
		"deepthroat",
		"onherknees",
		"blowjobsandwich",
		"iwanttosuckcock",
		"ass",
		"asstastic",
		"facedownassup",
		"assinthong",
		"bigasses",
		"buttplug",
		"TheUnderbun",
		"booty",
		"pawg",
		"paag",
		"cutelittlebutts",
		"hipcleavage",
		"frogbutt",
		"HungryButts",
		"cottontails",
		"lovetowatchyouleave",
		"celebritybutts",
		"cosplaybutts",
		"whooties",
		"booty_queens",
		"twerking",
		"anal",
		"analgw",
		"painal",
		"masterofanal",
		"buttsharpies",
		"asshole",
		"AssholeBehindThong",
		"assholegonewild",
		"spreadem",
		"godasshole",
		"girlsinyogapants",
		"yogapants",
		"boobies",
		"TittyDrop",
		"boltedontits",
		"boobbounce",
		"boobs",
		"downblouse",
		"homegrowntits",
		"cleavage",
		"breastenvy",
		"youtubetitties",
		"torpedotits",
		"thehangingboobs",
		"page3glamour",
		"fortyfivefiftyfive",
		"tits",
		"amazingtits",
		"titstouchingtits",
		"BustyPetite",
		"hugeboobs",
		"stacked",
		"burstingout",
		"BigBoobsGW",
		"bigboobsgonewild",
		"2busty2hide",
		"bigtiddygothgf",
		"engorgedveinybreasts",
		"bigtitsinbikinis",
		"biggerthanherhead",
		"pokies",
		"ghostnipples",
		"nipples",
		"puffies",
		"lactation",
		"tinytits",
		"aa_cups",
		"titfuck",
		"clothedtitfuck",
		"braceface",
		"GirlswithNeonHair",
		"shorthairchicks",
		"blonde",
		"girlsinyogapants",
		"stockings",
		"legs",
		"tightshorts",
		"buttsandbarefeet",
		"feet",
		"datgap",
		"thighhighs",
		"thickthighs",
		"thighdeology",
		"pussy",
		"rearpussy",
		"innie",
		"simps",
		"pelfie",
		"LabiaGW",
		"godpussy",
		"presenting",
		"cameltoe",
		"hairypussy",
		"pantiestotheside",
		"breakingtheseal",
		"moundofvenus",
		"pussymound",
		"Hotchickswithtattoos",
		"sexyfrex",
		"tanlines",
		"oilporn",
		"ComplexionExcellence",
		"SexyTummies",
		"theratio",
		"bodyperfection",
		"samespecies",
		"athleticgirls",
		"coltish",
		"gonewildcurvy",
		"curvy",
		"gonewildplus",
		"thick",
		"juicyasians",
		"voluptuous",
		"biggerthanyouthought",
		"jigglefuck",
		"chubby",
		"SlimThick",
		"massivetitsnass",
		"thicker",
		"thickthighs",
		"tightsqueeze",
		"casualjiggles",
		"bbw",
		"gonewildchubby",
		"amazingcurves",
		"fitgirls",
		"fitnakedgirls",
		"BustyPetite",
		"dirtysmall",
		"petitegonewild",
		"xsmallgirls",
		"funsized",
		"hugedicktinychick",
		"petite",
		"skinnytail",
		"volleyballgirls",
		"Ohlympics",
		"celebnsfw",
		"WatchItForThePlot",
		"nsfwcelebarchive",
		"celebritypussy",
		"oldschoolcoolNSFW",
		"extramile",
		"jerkofftocelebs",
		"celebritybutts",
		"onoffcelebs",
		"celebswithbigtits",
		"youtubersgonewild",
		"cumsluts",
		"GirlsFinishingTheJob",
		"cumfetish",
		"amateurcumsluts",
		"cumcoveredfucking",
		"cumhaters",
		"thickloads",
		"before_after_cumsluts",
		"pulsatingcumshots",
		"impressedbycum",
		"creampies",
		"throatpies",
		"FacialFun",
		"cumonclothes",
		"oralcreampie",
		"creampie",
		"HappyEmbarrassedGirls",
		"unashamed",
		"borednignored",
		"annoyedtobenude",
		"damngoodinterracial",
		"AsianHotties",
		"AsiansGoneWild",
		"realasians",
		"juicyasians",
		"AsianNSFW",
		"nextdoorasians",
		"asianporn",
		"bustyasians",
		"paag",
		"IndianBabes",
		"indiansgonewild",
		"NSFW_Japan",
		"javdownloadcenter",
		"kpopfap",
		"NSFW_Korea",
		"WomenOfColor",
		"darkangels",
		"blackchickswhitedicks",
		"ebony",
		"Afrodisiac",
		"ginger",
		"redheads",
		"latinas",
		"latinasgw",
		"latinacuties",
		"palegirls",
		"pawg",
		"snowwhites",
		"whooties",
		"NSFW_GIF",
		"nsfw_gifs",
		"porn_gifs",
		"porninfifteenseconds",
		"CuteModeSlutMode",
		"60fpsporn",
		"NSFW_HTML5",
		"the_best_nsfw_gifs",
		"verticalgifs",
		"besthqporngifs",
		"twingirls",
		"groupofnudegirls",
		"Ifyouhadtopickone",
		"nsfwhardcore",
		"SheLikesItRough",
		"strugglefucking",
		"jigglefuck",
		"whenitgoesin",
		"outercourse",
		"gangbang",
		"pegging",
		"insertions",
		"passionx",
		"xsome",
		"shefuckshim",
		"cuckold",
		"cuckquean",
		"breeding",
		"forcedcreampie",
		"hugedicktinychick",
		"amateurgirlsbigcocks",
		"bbcsluts",
		"facesitting",
		"nsfw_plowcam",
		"pronebone",
		"facefuck",
		"girlswhoride",
		"60fpsporn",
		"highresNSFW",
		"NSFW_HTML5",
		"incestporn",
		"wincest",
		"incest_gifs",
		"remylacroix",
		"Anjelica_Ebbi",
		"BlancNoir",
		"rileyreid",
		"tessafowler",
		"lilyivy",
		"mycherrycrush",
		"gillianbarnes",
		"emilybloom",
		"miamalkova",
		"sashagrey",
		"angelawhite",
		"miakhalifa",
		"alexapearl",
		"missalice_18",
		"lanarhoades",
		"evalovia",
		"GiannaMichaels",
		"erinashford",
		"sextrophies",
		"sabrina_nichole",
		"LiyaSilver",
		"MelissaDebling",
		"AdrianaChechik",
		"abelladanger",
		"sarah_xxx",
		"dollywinks",
		"funsizedasian",
		"Kawaiiikitten",
		"legendarylootz",
		"sexyflowerwater",
		"keriberry_420",
		"justpeachyy",
		"hopelesssofrantic",
		"lesbians",
		"StraightGirlsPlaying",
		"girlskissing",
		"mmgirls",
		"dykesgonewild",
		"justfriendshavingfun",
		"holdthemoan",
		"O_faces",
		"jilling",
		"gettingherselfoff",
		"quiver",
		"GirlsHumpingThings",
		"forcedorgasms",
		"mmgirls",
		"ruinedorgasms",
		"realahegao",
		"suctiondildos",
		"baddragon",
		"grool",
		"squirting",
		"ladybonersgw",
		"massivecock",
		"chickflixxx",
		"gaybrosgonewild",
		"sissies",
		"penis",
		"monsterdicks",
		"thickdick",
		"freeuse",
		"fuckdoll",
		"degradingholes",
		"fuckmeat",
		"OnOff",
		"nsfwoutfits",
		"girlswithglasses",
		"collared",
		"seethru",
		"sweatermeat",
		"cfnm",
		"nsfwfashion",
		"leotards",
		"whyevenwearanything",
		"shinyporn",
		"gothsluts",
		"bikinis",
		"bikinibridge",
		"bigtitsinbikinis",
		"nsfwcosplay",
		"nsfwcostumes",
		"girlsinschooluniforms",
		"WtSSTaDaMiT",
		"tightdresses",
		"upskirt",
		"SchoolgirlSkirts",
		"stockings",
		"thighhighs",
		"leggingsgonewild",
		"Bottomless_Vixens",
		"tightshorts",
		"tight_shorts",
		"girlsinyogapants",
		"yogapants",
		"lingerie",
		"pantiestotheside",
		"assinthong",
		"AssholeBehindThong",
		"porn",
		"suicidegirls",
		"GirlsDoPorn",
		"pornstarhq",
		"porninaminute",
		"ChangingRooms",
		"workgonewild",
		"FlashingGirls",
		"publicflashing",
		"sexinfrontofothers",
		"NotSafeForNature",
		"gwpublic",
		"realpublicnudity",
		"flashingandflaunting",
		"publicsexporn",
		"nakedadventures",
		"socialmediasluts",
		"slutsofsnapchat",
		"OnlyFans101",
		"tiktoknsfw",
		"tiktokthots",
		"tiktokporn",
		"trashyboners",
		"flubtrash",
		"realsexyselfies",
		"nude_selfie",
		"Tgirls",
		"traps",
		"futanari",
		"gonewildtrans",
		"tgifs",
		"shemales",
		"femboys",
		"transporn",
		"pornvids",
		"nsfw_videos",
		"dirtysnapchat",
		"randomactsofblowjob",
		"NSFWFunny",
		"pornhubcomments",
		"confusedboners",
		"dirtykikpals",
		"nsfw_wtf",
		"randomactsofmuffdive",
		"stupidslutsclub",
		"sluttyconfessions",
		"jobuds",
		"drunkdrunkenporn"
};


int getRandomNumber(int min, int max) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distr(min, max);
	return distr(gen);
}

std::string getRandomSubreddit(const std::vector<std::string> &subreddits) {
	int index = getRandomNumber(0, subreddits.size() - 1);
	return subreddits[index];
}

json getRandomPost(const json &valid_posts) {
	if (valid_posts.empty())
		return json();

	int index = getRandomNumber(1, valid_posts.size() - 1);
	if (valid_posts.contains(std::to_string(index))) {
		return valid_posts[std::to_string(index)];
	}
	return json();
}

json extractValidPosts(const json& response) {
	json valid_posts;
	int count = 0;

	for (const auto& child : response["data"]["children"]) {
		const json& data = child["data"];
		auto imageUrlIt = data.find("url_overridden_by_dest");

		if (imageUrlIt != data.end() && imageUrlIt->get<std::string>().find("i.redd.it") != std::string::npos) {
			json post = {
					{"author", data.value("author", "N/A")},
					{"description", data.value("selftext", "N/A")},
					{"image", *imageUrlIt},
					{"subreddit", data.value("subreddit", "N/A")}
			};
			valid_posts[std::to_string(++count)] = post;
		}
	}

	return valid_posts;
}

std::string getRandomTimePeriod() {
	std::vector<std::string> choices = {"month", "year", "all"};
	int index = getRandomNumber(0, choices.size() - 1);
	return choices[index];
}

json fetchSubredditData(const std::string& subreddit, const std::string& top, int limit) {
	std::string url = fmt::format("https://old.reddit.com/r/{}/top/.json?sort=top&limit={}&t={}", subreddit, limit, top);
	std::vector<std::string> headers = {
			"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:128.0) Gecko/20100101 Firefox/128.0",
			"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/png,image/svg+xml,*/*;q=0.8"
	};

	CringeCurl curl;
	return curl.get(url, headers);
}

void reddit_command(CringeBot &cringe, const dpp::slashcommand_t &event) {
	event.thinking();
	CringeCurl curl;

	std::string subreddit = std::get<std::string>(event.get_parameter("subreddit"));
	std::string top = std::get<std::string>(event.get_parameter("top"));

	int limit = (top == "random") ? 1000 : 25;

	try {
		if (subreddit == "random") {
			subreddit = getRandomSubreddit(SUBREDDITS);
		}
		if (top == "random") {
			top = getRandomTimePeriod();
		}

		json response = fetchSubredditData(subreddit, top, limit);
		json valid_posts = extractValidPosts(response);
		json post = getRandomPost(valid_posts);

		if (post.empty()) {
			CringeEmbed cringe_embed;
			cringe_embed.setTitle("Reddit Viewer").setHelp("view subreddits with /reddit!");
			cringe_embed.setColor(CringeColor::CringeError);
			cringe_embed.setFields({{"Error", "No valid posts could be obtained!", "false"}});
			dpp::message msg(event.command.channel_id, cringe_embed.embed);
			event.edit_original_response(msg);
			return;
		}
		CringeEmbed cringe_embed;
		cringe_embed.setTitle("Reddit Viewer").setHelp("view subreddits with /reddit!");
		cringe_embed.setFields(
				{
						{"Subreddit",   post["subreddit"],   "true"},
						{"Author",      post["author"],      "true"},
						{"description", post["description"], "false"}
				}
		);
		cringe_embed.setImage(post["image"]);
		dpp::message msg(event.command.channel_id, cringe_embed.embed);
		event.edit_original_response(msg);
	} catch (const json::invalid_iterator &e) {
		CringeEmbed cringe_embed;
		cringe_embed.setTitle("Reddit Viewer").setHelp("view subreddits with /reddit!");
		cringe_embed.setColor(CringeColor::CringeError);
		cringe_embed.setFields({{"Error!!!", fmt::format("{}", "you stupid idiot. KLIM!"), "false"}});
		dpp::message msg(event.command.channel_id, cringe_embed.embed);
		event.edit_original_response(msg);
		return;
	}

}
