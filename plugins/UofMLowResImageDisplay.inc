<?php
// $Id$

class UofMLowResImageDisplay {

  private $pid = NULL;
  private $item = NULL;

  function __construct($pid) {
    //drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
    $this->pid = $pid;
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    $this->item = new Fedora_Item($this->pid);
  }
  
  function showViewer()
  {
    global $base_url;
    $path = drupal_get_path('module', 'fedora_repository');
    
    module_load_include('inc', 'fedora_repository', 'plugins/ShowStreamsInFieldSets');
    module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    
    if ( ($cm = ContentModel::loadFromObject($this->pid)) !== FALSE) 
    {
      $related = get_related_items_as_array($this->pid, 'isPartOf', 10000, 0, TRUE, NULL, '$object');
      if ($cm->pid != 'uofm:prairieIslamicBuilding' && count($related) > 0) {
	array_unshift($related, $this->pid);
      }

      $path=drupal_get_path('module','fedora_repository');
      drupal_add_css($path.'/plugins/css/lowResViewer.css');

      

      $html='';
      if (isset($_SESSION['islandora_solr_query'])) {

	$html .= '<a href="/islandora/solr/search/'.$_SESSION['islandora_solr_query'].'/1/'.$_SESSION['islandora_solr_fq'].($_SESSION['islandora_solr_defType']=='dismax'?'/dismax':'').'?limit='.$_SESSION['islandora_solr_limit'].(isset($_SESSION['islandora_solr_sort'])?'&sort='.$_SESSION['islandora_solr_sort']:'').'&page='.$_SESSION['islandora_solr_page_number'].'">Back to Search Results</a>';

      }

      $html .= '<div class="viewer">';
      $html .= '<div class="toolbar"><div class="title"><h3>'.$this->item->objectProfile->objLabel.'</h3></div>
      <button id="toggle" class="text ui-corner-all ui-state-active" title="Metadata"><span class="ui-icon iiv-icon-text">Metadata</span></button>
      <button id="download" class="download ui-corner-all ui-state-active" title="Download Version"><span class="ui-icon iiv-icon-download">Download Version</span></button>
      <button id="print" class="-r9jt ui-corner-all ui-state-active" title="Print Version"><span class="ui-icon iiv-icon-print">Print Version</span></button>';
      
      if (count($related) > 0) {
	$html .='<div class="pageControls">
	<button id="next" class="nextPage ui-corner-all ui-state-active" title="Next"><span class="ui-icon iiv-icon-nextPage">Next</span></button>
	<div class="pageText">1 of '.count($related).'</div>
	<button id="prev" class="prevPage ui-corner-all ui-state-active" title="Previous"><span class="ui-icon iiv-icon-prevPage">Previous</span></button>
	</div>';
      }
      
      $html .= '</div>';
      $html .= '<div id="contentPane"'.(count($related)>0?' class="thumbs"':'').'>';
      
      $selectedPid = $this->pid;
      if (isset($this->item->datastreams['JPG'])) { 
	$html .= '<a href="'.$base_url.'/fedora/repository/'.$this->pid.'/OBJ" target="_new" class="fullSize"><img src="'.$base_url.'/fedora/repository/'.$this->pid.'/JPG" border="0"></a>';
      } else if (count($related) > 0) { 
	$selectedPid = $related[0];
	$html .= '<a href="'.$base_url.'/fedora/repository/'.$related[0].'/OBJ" target="_new" class="fullSize"><img src="'.$base_url.'/fedora/repository/'.$related[0].'/JPG" border="0"></a>';
      }
      $html.='</div>';
      
      if (count($related) > 0) {
	$html.='<div class="thumbnailsPane">';
	foreach ($related as $pid) {
	  $html.='<img src="'.$base_url.'/fedora/repository/'.$pid.'/TN" width="100" class="thumb'.($pid==$selectedPid?' selected':'').'" id="'.$pid.'"><br/>';
	}
	$html.='</div>';
      }

      
      $html .= '<div class="metadataPane"><div class="text"><div id="pageText">';
      if (!isset($this->item->datastreams['JPG'])) { 
	$first_item = new Fedora_Item($related[0]); 
	$html .= $first_item->get_dissemination('uofm:mods2htmlSdef','mods2html');
      }
      $html .='</div>';
      $html .= $this->item->get_dissemination('uofm:mods2htmlSdef','mods2html');
      $html .= '</div></div></div><br clear="all"/>';
      
       $fedora_base = variable_get('fedora_base_url', 'http://localhost:8080/fedora');
       if (preg_match('/^(http.*\/).*/',$fedora_base,$matches)) {
 	$fedora_base = ($matches[1]);
       }
       
drupal_add_js("$(document).ready(function (){
	var metadataHidden = false;
	var width; 
	var height;
	var pid = '{$this->pid}';
	var curPid = '{$selectedPid}';
	var curIndex = 0;
	var maxIndex = ".(count($related)-1).";
	
	
	var resizeImage = function ( name ) {
      var newWidth=width; var newHeight=height;
      var maxHeight = $('#contentPane').height();
      var maxWidth = $('#contentPane').width();

      
      if (height > maxHeight) {
	newWidth=width*maxHeight/height;
	newHeight=maxHeight;
      }
      if (newWidth > maxWidth) {
	newHeight=newHeight*maxWidth/newWidth;
	newWidth=maxWidth;
      }
      
      $(name).width(newWidth+'px');
      $(name).height(newHeight+'px');


      if (newWidth < maxWidth) {
	$(name).css('margin-left', ((maxWidth-newWidth)/2)+'px');
      }
      
      if (newHeight < maxHeight) {
	$(name).css('margin-top', ((maxHeight-newHeight)/2)+'px');
      }
      
    };
    
    var changeImages = function (newPid) { 
      $('#contentPane .fullSize').remove();
      $('#contentPane').append('<a href=\"/fedora/repository/'+newPid+'/OBJ\" target=\"_new\" class=\"fullSize\"><img src=\"/fedora/repository/'+newPid+'/JPG\" border=\"0\"/></a>');
      curPid = newPid;
      if (curPid == pid) {
	$('.metadataPane #pageText').html(null);
      } else {
	$.get('/fedora/repository/service/'+curPid+'/uofm:mods2htmlSdef/mods2html', function (data) { $('.metadataPane #pageText').html(data); });
      }
      
      $('#contentPane img').load(function () { 
	width=$(this).width();
	height=$(this).height();
	resizeImage(this); 
      });
      var i =0;
      $('.thumb').each(function () {
	$(this).removeClass('selected'); 
	if ($(this).attr('id') == newPid) {
	  curIndex=i;
	  $(this).addClass('selected');    
	}
	i++;
      });
      
      $('.pageText').html( (curIndex+1)+' of '+(maxIndex+1) );
    };
    
    $('.text ul li:odd').addClass('alternate');
    
    $('#next').click(function () { 
      var i=0;
      var nextIndex = Math.min(curIndex+1,maxIndex);
      if (nextIndex != curIndex)
      {
	curIndex=nextIndex;    
	$('.thumb').each(function () { 
	  if (i == curIndex) {
	    changeImages($(this).attr('id'));
	  }

	  i++;
	}); 

      }
    });


    $('#prev').click(function () { 
      var i=0;
      var nextIndex = Math.max(curIndex-1,0);
      if (nextIndex != curIndex)
      {
	curIndex=nextIndex;
	$('.thumb').each(function () { 
	  if (i == curIndex) {
	    changeImages($(this).attr('id'));
	  }

	  i++;
	}); 
      }
    });


    $('#toggle').click(function () { 
      if (metadataHidden) {
	$('#contentPane').width( $('#contentPane').width() - $('.metadataPane').width()+'px');      
	$(this).removeClass('ui-state-default');
	$(this).addClass('ui-state-active');
      } else {
	$('#contentPane').width( $('#contentPane').width() + $('.metadataPane').width()+'px');      
	$(this).removeClass('ui-state-active');
	$(this).addClass('ui-state-default');     
      }
      
      metadataHidden= !metadataHidden;
      $('.metadataPane').toggle();

      $('#contentPane img').width(width+'px'); 
      $('#contentPane img').height(height+'px'); 
      $('#contentPane img').css('margin-left','0px');
      $('#contentPane img').css('margin-top','0px');
      resizeImage('#contentPane img');
      
    });
    $('#download').click(function () { document.location = '/fedora/repository/object_download/'+curPid+'/OBJ'; } );
    $('#print').click(function () { 
      
      // open popup window
      var popupWidth = Math.max(800, Math.min(624, window.screen.availWidth));
      var popupHeight = Math.max(600, Math.min(824 / 2, window.screen.availHeight));
      var features = 'width=' + popupWidth + ',height=' + popupHeight + ',toolbar=1,scrollbars=yes';
      window.open('{$fedora_base}iiv/print.html?pid='+curPid+'&image={$base_url}/fedora/repository/object_download/'+curPid+'/JPG', '_blank', features);
    } );
    $('.thumb').click(function () { 
      changeImages($(this).attr('id'));
    });
    $('#contentPane img').load(function () { 
      width=$(this).width();
      height=$(this).height();
      resizeImage(this); 
    });
  }); ",'inline');

      if (user_access(ObjectHelper :: $PURGE_FEDORA_OBJECTSANDSTREAMS)) {
	
	$html .= '<a title="'. t('Edit Metadata') . $this->pid . '" href="'. base_path() . 'fedora/repository/editmetadata/'. 
	  $this->pid . '/MODS"><img src="'. $base_url . '/'. $path . '/images/edit.gif" alt="'. t('Edit Metadata') . '" class="icon">'.t('Edit Metadata').'</a>';
      }
    }
   
    return $html;
    
  }
  
  function showParent()
  {
      $query_string = 'select $parent $parentTitle $object $title $content from <#ri>
		       where ($object <dc:title> $title
		       and $object <fedora-model:hasModel> $content
                       and $child <fedora-rels-ext:isPartOf> $parent
                       and $parent <dc:title> $parentTitle
                       and $child <dc:identifier> \''.$this->pid.'\'
		       and $object <fedora-rels-ext:isPartOf> $parent 
		       and $object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>)
		       minus $content <mulgara:is> <info:fedora/fedora-system:FedoraObject-3.0>
		       order by $object';     
		       
     return $this->resourceIndexQuery($query_string);
  }
  
  function showPartOfImages()
  {
   return $this->resourceIndexQuery($query_string);
  }
  
 
}
