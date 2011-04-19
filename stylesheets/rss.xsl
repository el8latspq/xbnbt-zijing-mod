<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:torrentItem="http://xbnbt.sourceforge.net/ns/torrentItem#">
	<xsl:output method="html" encoding="utf-8" />
	<xsl:template match="/">
		<html lang="zh">
			<head>
				<title>
					<xsl:value-of select="//channel/title" />
				</title>
				<META name="robots" content="noindex" />
				<META name="MSSmartTagsPreventParsing" content="true" />
			</head>
			<body>
				<xsl:for-each select="//channel">
					<div>
						<xsl:text>Channel Image: </xsl:text>
						<a href="{image/link}" title="{image/link}">
							<img src="{image/url}" title="{image/title}" alt="{image/title}" width="{image/width}"
								height="{image/height}" name="{image/title}" />
						</a>
					</div>
					<div>
						<xsl:text>Channel Title: </xsl:text>
						<xsl:value-of select="title" />
					</div>
					<div>
						<xsl:text>Channel Description: </xsl:text>
						<xsl:value-of select="description" />
					</div>
					<div>
						<xsl:text>Channel Published: </xsl:text>
						<xsl:value-of select="pubDate" />
					</div>
					<div>
						<xsl:text>Channel Last Build: </xsl:text>
						<xsl:value-of select="lastBuildDate" />
					</div>
					<div>
						<xsl:text>Channel Category: </xsl:text>
						<a href="{category/@domain}" title="{category/@domain}">
							<xsl:value-of select="category" />
						</a>
					</div>
					<xsl:for-each select="item">
						<br></br>
						<div>
							<xsl:text>Item Title: </xsl:text>
							<xsl:value-of select="title" />
						</div>
						<div>
							<div>
								<xsl:text>Item Infostat Title: </xsl:text>
								<xsl:value-of select="torrentItem:infostat/@title" />
							</div>
							<div>
								<xsl:text>Item Link: </xsl:text>
								<a href="{link}" title="{link}">
									<xsl:value-of select="link" />
								</a>
							</div>
						</div>
						<div>
							<div>
								<xsl:text>Item Download Title: </xsl:text>
								<xsl:value-of select="torrentItem:download/@title" />
							</div>
							<div>
								<xsl:text>Item Enclosure URL: </xsl:text>
								<a href="{enclosure/@url}" title="{enclosure/@url}">
									<xsl:value-of select="enclosure/@url" />
								</a>
							</div>
							<div>
								<xsl:text>Item Enclosure Length: </xsl:text>
								<xsl:value-of select="enclosure/@length" />
							</div>
							<div>
								<xsl:text>Item Enclosure Type: </xsl:text>
								<xsl:value-of select="enclosure/@type" />
							</div>
						</div>
						<div>
							<xsl:text>Item Author: </xsl:text>
							<a href="mailto:{author}" title="{author}">
								<xsl:value-of select="author" />
							</a>
						</div>
						<div>
							<xsl:text>Item Published: </xsl:text>
							<xsl:value-of select="pubDate" />
						</div>
						<div>
							<xsl:text>Item Category: </xsl:text>
							<a href="{category/@domain}" title="{category/@domain}">
								<xsl:value-of select="category" />
							</a>
						</div>
						<div>
							<xsl:text>Item Description: </xsl:text>
							<xsl:value-of select="description" />
						</div>
						<div>
							<div>
								<xsl:text>Item Size Title: </xsl:text>
								<xsl:value-of select="torrentItem:size/@title" />
							</div>
							<div>
								<xsl:text>Item Size: </xsl:text>
								<xsl:value-of select="torrentItem:size" />
							</div>
						</div>
						<div>
							<div>
								<xsl:text>Item Files Title: </xsl:text>
								<xsl:value-of select="torrentItem:files/@title" />
							</div>
							<div>
								<xsl:text>Item Files: </xsl:text>
								<xsl:value-of select="torrentItem:files" />
							</div>
						</div>
						<div>
							<div>
								<xsl:text>Item Seeders Title: </xsl:text>
								<xsl:value-of select="torrentItem:seeders/@title" />
							</div>
							<div>
								<xsl:text>Item Seeders: </xsl:text>
								<xsl:value-of select="torrentItem:seeders" />
							</div>
						</div>
						<div>
							<div>
								<xsl:text>Item Leechers Title: </xsl:text>
								<xsl:value-of select="torrentItem:leechers/@title" />
							</div>
							<div>
								<xsl:text>Item Leechers: </xsl:text>
								<xsl:value-of select="torrentItem:leechers" />
							</div>
						</div>
						<div>
							<div>
								<xsl:text>Item Completed Title: </xsl:text>
								<xsl:value-of select="torrentItem:completed/@title" />
							</div>
							<div>
								<xsl:text>Item Completed: </xsl:text>
								<xsl:value-of select="torrentItem:completed" />
							</div>
						</div>
						<div>
							<xsl:text>Item Infolink: </xsl:text>
							<a href="{torrentItem:infolink}" title="{torrentItem:infolink}">
								<xsl:value-of select="torrentItem:infolink/@title" />
							</a>
						</div>
						<div>
							<div>
								<xsl:text>Item Comments Title: </xsl:text>
								<xsl:value-of select="torrentItem:comments/@title" />
							</div>
							<div>
								<xsl:text>Item Comments: </xsl:text>
								<a href="{comments}" title="{comments}">
									<xsl:value-of select="comments" />
								</a>
							</div>
						</div>
						<div>
							<div>
								<xsl:text>Item Infohash Title: </xsl:text>
								<xsl:value-of select="torrentItem:infohash/@title" />
							</div>
							<div>
								<xsl:text>Item Guid: </xsl:text>
								<xsl:value-of select="guid" />
							</div>
						</div>
					</xsl:for-each>
					<br></br>
					<div>
						<xsl:text>Channel Text Input: </xsl:text>
						<form class="channel_textinput" name="textInput" method="get" action="{textInput/link}">
							<label for="id_text">
								<xsl:value-of select="textInput/description" />
							</label>
							<input name="{textInput/name}" id="id_text" alt="{textInput/name}" type="text" size="40"></input>
							<span style="display:none">
								<label for="id_submit">
									<xsl:value-of select="textInput/title" />
									<xsl:text>(</xsl:text>
									<xsl:value-of select="textInput/description" />
									<xsl:text>)</xsl:text>
								</label>
							</span>
							<input name="button" id="id_submit" alt="{textInput/title}" type="submit" value="{textInput/title}"></input>
						</form>
					</div>
					<div>
						<xsl:text>Channel Managing Editor: </xsl:text>
						<a href="mailto:{managingEditor}" title="{managingEditor}">
							<xsl:value-of select="managingEditor" />
						</a>
					</div>
					<div>
						<xsl:text>Channel Web Master: </xsl:text>
						<a href="mailto:{webMaster}" title="{webMaster}">
							<xsl:value-of select="webMaster" />
						</a>
					</div>
					<div>
						<xsl:text>Channel Generator: </xsl:text>
						<xsl:value-of select="generator" />
					</div>
					<div>
						<xsl:text>Channel Copyright: </xsl:text>
						<xsl:value-of select="copyright" />
					</div>
				</xsl:for-each>
			</body>
		</html>
	</xsl:template>
</xsl:stylesheet>
