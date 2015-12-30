<?php

// --- Selector.inc - (c) Copyright TJ Holowaychuk <tj@vision-media.ca> MIT Licensed

define('SELECTOR_VERSION', '1.1.6');

/**
 * SelectorDOM.
 *
 * Persitant object for selecting elements.
 *
 *   $dom = new SelectorDOM($html);
 *   $links = $dom->select('a');
 *   $list_links = $dom->select('ul li a');
 *
 */

class SelectorDOM implements \IteratorAggregate{
    /**
     * @var DomNode[]
     */
    private $nodes=[];

    /**
     * @var DOMDocument
     */
    private $document;

    protected static function NULL_DOM()
    {
        static $nullDom=null;
        if($nullDom === null){
            $nullDom = new SelectorDOM([]);
        }
        return $nullDom;
    }

    public function SelectorDOM($data) {
        if($data instanceof DomNode)
        {
            $data = [$data];
        }
        if(is_array($data)){
            $this->setNodes($data);
            if(isset($data[0])){
                $this->document = $data[0]->ownerDocument;
                $this->xpath = new DOMXPath($this->document);
            }
        }else if (is_string($data)) {
            $this->document = new DOMDocument();
            @$this->document->loadHTML($data);
            $this->setNodes([$this->document]);
            $this->xpath = new DOMXpath($this->document);
        }
    }

    public function getIterator()
    {
        return new ArrayIterator($this->selements());
    }

    protected function setNodes($nodes)
    {
        $this->nodes=[];
        foreach($nodes as $node){
            if($node instanceof DomNode) {
                $this->nodes[] = $node;
            }else if($node instanceof SelectorDOM) {
                $this->nodes = array_merge($this->nodes, $node->nodes);
            }
        }
    }

    public function elements()
    {
        return array_map(function($dom){return qd($dom);},$this->nodes());
    }

    public function nodes()
    {
        return $this->nodes;
    }

    public function get()
    {
        return $this->nodes[0];
    }

    public function html()
    {
        return $this->document->saveHTML($this->get());
    }

    public function text()
    {
        return trim($this->get()->textContent);
    }

    public function attr($attrName)
    {
        return $this->get()->getAttribute($attrName);
    }

    public function select($selector) {
        if(empty($this->nodes())){
            return self::NULL_DOM();
        }
        $domElements = [];
        foreach($this->nodes as $contextElement){
            foreach($this->xpath->evaluate(selector_to_xpath($selector),$contextElement) as $dom){
                $domElements[]=$dom;
            }
        }
        return new SelectorDOM($domElements);
    }
}

function qd($dom)
{
    if($dom instanceof SelectorDOM){
        return $dom;
    }
    return new SelectorDOM($dom);
}


/**
 * Convert $selector into an XPath string.
 */
function selector_to_xpath($selector) {
    // remove spaces around operators
    $selector = preg_replace('/\s*>\s*/', '>', $selector);
    $selector = preg_replace('/\s*~\s*/', '~', $selector);
    $selector = preg_replace('/\s*\+\s*/', '+', $selector);
    $selector = preg_replace('/\s*,\s*/', ',', $selector);
    $selectors = preg_split('/\s+(?![^\[]+\])/', $selector);

    foreach ($selectors as &$selector) {
        // ,
        $selector = preg_replace('/,/', '|descendant-or-self::', $selector);
        // input:checked, :disabled, etc.
        $selector = preg_replace('/(.+)?:(checked|disabled|required|autofocus)/', '\1[@\2="\2"]', $selector);
        // input:autocomplete, :autocomplete
        $selector = preg_replace('/(.+)?:(autocomplete)/', '\1[@\2="on"]', $selector);
        // input:button, input:submit, etc.
        $selector = preg_replace('/:(text|password|checkbox|radio|button|submit|reset|file|hidden|image|datetime|datetime-local|date|month|time|week|number|range|email|url|search|tel|color)/', 'input[@type="\1"]', $selector);
        // foo[id]
        $selector = preg_replace('/(\w+)\[([_\w-]+[_\w\d-]*)\]/', '\1[@\2]', $selector);
        // [id]
        $selector = preg_replace('/\[([_\w-]+[_\w\d-]*)\]/', '*[@\1]', $selector);
        // foo[id=foo]
        $selector = preg_replace('/\[([_\w-]+[_\w\d-]*)=[\'"]?(.*?)[\'"]?\]/', '[@\1="\2"]', $selector);
        // [id=foo]
        $selector = preg_replace('/^\[/', '*[', $selector);
        // div#foo
        $selector = preg_replace('/([_\w-]+[_\w\d-]*)\#([_\w-]+[_\w\d-]*)/', '\1[@id="\2"]', $selector);
        // #foo
        $selector = preg_replace('/\#([_\w-]+[_\w\d-]*)/', '*[@id="\1"]', $selector);
        // div.foo
        $selector = preg_replace('/([_\w-]+[_\w\d-]*)\.([_\w-]+[_\w\d-]*)/', '\1[contains(concat(" ",@class," ")," \2 ")]', $selector);
        // .foo
        $selector = preg_replace('/\.([_\w-]+[_\w\d-]*)/', '*[contains(concat(" ",@class," ")," \1 ")]', $selector);
        // div:first-child
        $selector = preg_replace('/([_\w-]+[_\w\d-]*):first-child/', '*/\1[position()=1]', $selector);
        // div:last-child
        $selector = preg_replace('/([_\w-]+[_\w\d-]*):last-child/', '*/\1[position()=last()]', $selector);
        // :first-child
        $selector = str_replace(':first-child', '*/*[position()=1]', $selector);
        // :last-child
        $selector = str_replace(':last-child', '*/*[position()=last()]', $selector);
        // :nth-last-child
        $selector = preg_replace('/:nth-last-child\((\d+)\)/', '[position()=(last() - (\1 - 1))]', $selector);
        // div:nth-child
        $selector = preg_replace('/([_\w-]+[_\w\d-]*):nth-child\((\d+)\)/', '*/*[position()=\2 and self::\1]', $selector);
        // :nth-child
        $selector = preg_replace('/:nth-child\((\d+)\)/', '*/*[position()=\1]', $selector);
        // :contains(Foo)
        $selector = preg_replace('/([_\w-]+[_\w\d-]*):contains\((.*?)\)/', '\1[contains(string(.),"\2")]', $selector);
        // >
        $selector = preg_replace('/>/', '/', $selector);
        // ~
        $selector = preg_replace('/~/', '/following-sibling::', $selector);
        // +
        $selector = preg_replace('/\+([_\w-]+[_\w\d-]*)/', '/following-sibling::\1[position()=1]', $selector);
        $selector = str_replace(']*', ']', $selector);
        $selector = str_replace(']/*', ']', $selector);
    }

    // ' '
    $selector = implode('/descendant::', $selectors);
    $selector = 'descendant-or-self::' . $selector;
    // :scope
    $selector = preg_replace('/(((\|)?descendant-or-self::):scope)/', '.\3', $selector);
    // $element
    $sub_selectors = explode(',', $selector);

    foreach ($sub_selectors as $key => $sub_selector) {
        $parts = explode('$', $sub_selector);
        $sub_selector = array_shift($parts);

        if (count($parts) && preg_match_all('/((?:[^\/]*\/?\/?)|$)/', $parts[0], $matches)) {
            $results = $matches[0];
            $results[] = str_repeat('/..', count($results) - 2);
            $sub_selector .= implode('', $results);
        }

        $sub_selectors[$key] = $sub_selector;
    }

    $selector = implode(',', $sub_selectors);

    return $selector;
}
