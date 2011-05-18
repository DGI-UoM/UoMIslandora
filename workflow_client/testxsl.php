<?
		$xsltDom = DOMDocument::load('xsl/mods-solr_prairieIslamicBuilding.xsl');
		$dom = DOMDocument::load('mods.xml');

    $proc = new XSLTProcessor();
    $proc->importStylesheet($xsltDom);
    $solrDom = $proc->transformToDOC($dom);

		echo $solrDom->saveXML();
?>
