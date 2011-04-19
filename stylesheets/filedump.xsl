<?xml version="1.0" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:template match="/">
		<html>
			<body>
				<xsl:for-each select="torrents/torrent">
					<table border="2" bgcolor="blue">
						<tr>
							<th bgcolor="cyan">Hash</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@hash" />
							</td>
						</tr>
						<tr>
							<th bgcolor="cyan">Torrent</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@filename" />
							</td>
						</tr>
						<tr>
							<th bgcolor="cyan">Name</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@name" />
							</td>
						</tr>
						<tr>
							<th bgcolor="cyan">Added</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@added" />
							</td>
						</tr>
						<tr>
							<th bgcolor="cyan">Size</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@size" />
							</td>
						</tr>
						<tr>
							<th bgcolor="cyan">Files</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@files" />
							</td>
						</tr>
						<tr>
							<th bgcolor="cyan">Completed</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@completed" />
							</td>
						</tr>
						<tr>
							<th bgcolor="cyan">Tag</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@tag" />
							</td>
						</tr>
						<tr>
							<th bgcolor="cyan">Description</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@uploadname" />
							</td>
						</tr>
						<tr>
							<th bgcolor="cyan">Uploader</th>
							<td style=".white-space:nowrap">
								<xsl:value-of select="@uploader" />
							</td>
						</tr>
					</table>
					<table border="2" bgcolor="blue">
						<tr>
							<th bgcolor="cyan">ID</th>
							<th bgcolor="cyan">IP</th>
							<th bgcolor="cyan">Uploaded</th>
							<th bgcolor="cyan">Downloaded</th>
							<th bgcolor="cyan">Left</th>
							<th bgcolor="cyan">Connected</th>
						</tr>
						<xsl:for-each select="peers/peer">
							<tr>
								<td style="white-space:nowrap">
									<xsl:value-of select="@id" />
								</td>
								<td style="white-space:nowrap">
									<xsl:value-of select="@ip" />
								</td>
								<td style="white-space:nowrap">
									<xsl:value-of select="@uploaded" />
								</td>
								<td style="white-space:nowrap">
									<xsl:value-of select="@downloaded" />
								</td>
								<td style="white-space:nowrap">
									<xsl:value-of select="@left" />
								</td>
								<td style="white-space:nowrap">
									<xsl:value-of select="@connected" />
								</td>
							</tr>
						</xsl:for-each>
					</table>
					<p></p>
				</xsl:for-each>
			</body>
		</html>
	</xsl:template>
</xsl:stylesheet>